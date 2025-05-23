/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "catapult/model/StorageNotifications.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DataModificationApprovalValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(DataModificationApproval, )

    namespace {
        using Notification = model::DataModificationApprovalNotification<1>;

        constexpr auto Current_Height = Height(10);
		constexpr auto Min_Replicator_Count = 4;
		constexpr auto Replicator_Count = 5;
		constexpr auto Required_Signatures_Count = Replicator_Count * 2 / 3 + 1;
		constexpr auto Common_Data_Size = sizeof(Key) + 2 * sizeof(Hash256) + 3 * sizeof(uint64_t);
		const uint8_t Modification_Status = 0;
		const auto File_Structure_Cdi =  test::GenerateRandomByteArray<Hash256>();
        constexpr auto File_Structure_Size = 50;
		constexpr auto Meta_Files_Size = 50;
        constexpr auto Used_Drive_Size = 50;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::StorageConfiguration::Uninitialized();
			pluginConfig.MinReplicatorCount = Min_Replicator_Count;
			config.Network.SetPluginConfiguration(pluginConfig);
			return (config.ToConst());
		}

		void PrepareDriveEntry(state::BcDriveEntry& driveEntry, const std::vector<crypto::KeyPair>& replicatorKeyPairs) {
			for (const auto& pair : replicatorKeyPairs)
				driveEntry.replicators().insert(pair.publicKey());
		}

        void AssertValidationResult(
                ValidationResult expectedResult,
				const cache::CatapultCache& cache,
                const Key& driveKey,
				const Hash256& dataModificationId,
				const test::OpinionData<uint64_t>& opinionData) {
            // Arrange:
			const auto totalJudgingKeysCount = opinionData.JudgingKeysCount + opinionData.OverlappingKeysCount;
			const auto totalJudgedKeysCount = opinionData.OverlappingKeysCount + opinionData.JudgedKeysCount;
			const auto presentOpinionElementCount = totalJudgingKeysCount * totalJudgedKeysCount;
			const auto presentOpinionByteCount = (presentOpinionElementCount + 7) / 8;

			const auto pPublicKeys = std::unique_ptr<Key[]>(new Key[opinionData.PublicKeys.size()]);
			for (auto i = 0u; i < opinionData.PublicKeys.size(); ++i)
				memcpy(static_cast<void*>(&pPublicKeys[i]), static_cast<const void*>(&opinionData.PublicKeys.at(i)), sizeof(Key));
			const auto pPresentOpinions = std::unique_ptr<uint8_t[]>(new uint8_t[presentOpinionByteCount]);
			for (auto i = 0u; i < presentOpinionByteCount; ++i) {
				boost::dynamic_bitset<uint8_t> byte(8, 0u);
				for (auto j = 0u; j < std::min(8u, presentOpinionElementCount - i*8); ++j) {
					const auto bitNumber = i*8 + j;
					byte[j] = opinionData.PresentOpinions.at(bitNumber / totalJudgedKeysCount).at(bitNumber % totalJudgedKeysCount);
				}
				boost::to_block_range(byte, &pPresentOpinions[i]);
			}

            Notification notification(
					driveKey,
					dataModificationId,
					File_Structure_Cdi,
					Modification_Status,
					File_Structure_Size,
					Meta_Files_Size,
					Used_Drive_Size,
					opinionData.JudgingKeysCount,
					opinionData.OverlappingKeysCount,
					opinionData.JudgedKeysCount,
					pPublicKeys.get(),
					pPresentOpinions.get()
			);
            auto pValidator = CreateDataModificationApprovalValidator();
            
            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfig(), Current_Height);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }

		void AssertValidationResultWithoutOpinionData(
				ValidationResult expectedResult,
				const cache::CatapultCache& cache,
				const Key& driveKey,
				const Hash256& dataModificationId,
				const std::vector<crypto::KeyPair>& replicatorKeyPairs,
				const bool includeDriveOwner = true) {
			// Generate valid opinion data:
			const auto totalJudgingKeysCount = test::RandomInRange(Required_Signatures_Count, Replicator_Count);
			const auto judgedKeysCount = Replicator_Count - totalJudgingKeysCount;
			const auto overlappingKeysCount = test::RandomInRange(judgedKeysCount ? 0 : 1, totalJudgingKeysCount);
			const auto judgingKeysCount = totalJudgingKeysCount - overlappingKeysCount;

			const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size);
			auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer,
					{ judgingKeysCount, overlappingKeysCount, judgedKeysCount });
			delete[] commonDataBuffer.pData;

			// If includeDriveOwner is enabled and there are any judged keys, replace one of them with Drive Owner key with probability 0.5:
			if (includeDriveOwner && opinionData.JudgedKeysCount && test::RandomByte() % 2) {
				auto driveIter = cache.createView().sub<cache::BcDriveCache>().find(driveKey);
				const auto& driveEntry = driveIter.get();
				const auto targetKeyIndex = test::RandomInRange(totalJudgingKeysCount, totalJudgingKeysCount + opinionData.JudgedKeysCount - 1);
				opinionData.PublicKeys.at(targetKeyIndex) = driveEntry.owner();
			}

			// Make sure that every judging replicator has NOT provided an opinion on self:
			for (auto i = opinionData.JudgingKeysCount; i < totalJudgingKeysCount; ++i)
				opinionData.PresentOpinions.at(i).at(i - opinionData.JudgingKeysCount) = false;

			// Pass:
			AssertValidationResult(
					expectedResult,
					cache,
					driveKey,
					dataModificationId,
					opinionData);
		}
    }

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::PopulateReplicatorKeyPairs(replicatorKeyPairs, Replicator_Count);
		PrepareDriveEntry(driveEntry, replicatorKeyPairs);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				ValidationResult::Success,
				cache,
				driveEntry.key(),
				driveEntry.activeDataModifications().begin()->Id,
				replicatorKeyPairs);
	}

    TEST(TEST_CLASS, FailureWhenDriveNotFound) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		// No need to create drive delta and drive entry.

		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::PopulateReplicatorKeyPairs(replicatorKeyPairs, Replicator_Count);
		// No need to prepare drive entry.

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Drive_Not_Found,
				cache,
				test::GenerateRandomByteArray<Key>(),
				test::GenerateRandomByteArray<Hash256>(),
				replicatorKeyPairs,
				false);
    }

    TEST(TEST_CLASS, FailureWhenNoActiveDataModification) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::PopulateReplicatorKeyPairs(replicatorKeyPairs, Replicator_Count);
		PrepareDriveEntry(driveEntry, replicatorKeyPairs);

		// Act:
		driveEntry.activeDataModifications().clear();

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_No_Active_Data_Modifications,
				cache,
				driveEntry.key(),
				test::GenerateRandomByteArray<Hash256>(),
				replicatorKeyPairs);
    }

    TEST(TEST_CLASS, FailureWhenInvalidDataModificationId) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::PopulateReplicatorKeyPairs(replicatorKeyPairs, Replicator_Count);
		PrepareDriveEntry(driveEntry, replicatorKeyPairs);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Invalid_Data_Modification_Id,
				cache,
				driveEntry.key(),
				test::GenerateRandomByteArray<Hash256>(),
				replicatorKeyPairs);
    }

	TEST(TEST_CLASS, FailureWhenSignatureCountInsufficient) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::PopulateReplicatorKeyPairs(replicatorKeyPairs, Replicator_Count);
		PrepareDriveEntry(driveEntry, replicatorKeyPairs);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		const auto totalJudgingKeysCount = test::RandomInRange(1, Required_Signatures_Count-1);
		const auto overlappingKeysCount = test::RandomInRange(0, totalJudgingKeysCount);
		const auto judgingKeysCount = totalJudgingKeysCount - overlappingKeysCount;
		const auto judgedKeysCount = Replicator_Count - totalJudgingKeysCount;
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {judgingKeysCount, overlappingKeysCount, judgedKeysCount});
		delete[] commonDataBuffer.pData;

		// Assert:
		AssertValidationResult(
				Failure_Storage_Signature_Count_Insufficient,
				cache,
				driveEntry.key(),
				driveEntry.activeDataModifications().begin()->Id,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenOpinionProvidedOnSelf) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();
		driveEntry.activeDataModifications().begin()->ReadyForApproval = true;

		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::PopulateReplicatorKeyPairs(replicatorKeyPairs, Replicator_Count);
		PrepareDriveEntry(driveEntry, replicatorKeyPairs);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		const auto totalJudgingKeysCount = test::RandomInRange(Required_Signatures_Count, Replicator_Count);
		const auto overlappingKeysCount = test::RandomInRange(1, totalJudgingKeysCount);	// Need at least one overlapping key.
		const auto judgingKeysCount = totalJudgingKeysCount - overlappingKeysCount;
		const auto judgedKeysCount = Replicator_Count - totalJudgingKeysCount;
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {judgingKeysCount, overlappingKeysCount, judgedKeysCount});
		delete[] commonDataBuffer.pData;

		// Act:
		const auto targetKeyIndex = test::RandomInRange<uint8_t>(judgingKeysCount, totalJudgingKeysCount-1);
		opinionData.PresentOpinions.at(targetKeyIndex).at(targetKeyIndex - judgingKeysCount) = true;

		// Assert:
		AssertValidationResult(
				Failure_Storage_Opinion_Provided_On_Self,
				cache,
				driveEntry.key(),
				driveEntry.activeDataModifications().begin()->Id,
				opinionData);
	}
}}