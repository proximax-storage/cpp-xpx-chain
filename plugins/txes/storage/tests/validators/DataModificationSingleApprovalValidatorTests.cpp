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

#define TEST_CLASS DataModificationSingleApprovalValidatorTests

    DEFINE_COMMON_VALIDATOR_TESTS(DataModificationSingleApproval, )

    namespace {
        using Notification = model::DataModificationSingleApprovalNotification<1>;

        constexpr auto Current_Height = Height(10);
		constexpr auto Public_Keys_Count = 5;
        constexpr auto Used_Drive_Size = 50;

		void PrepareDriveEntry(
				state::BcDriveEntry& driveEntry,
				const Key& signer,
				const Hash256& dataModificationId,
				const int16_t ownerIndex,
				const std::vector<Key>& publicKeys) {
			driveEntry.replicators().insert(signer);

			auto& lastCompletedModification = driveEntry.completedDataModifications().back();
			lastCompletedModification.Id = dataModificationId;
			lastCompletedModification.State = state::DataModificationState::Succeeded;

			if (ownerIndex != -1) {
				driveEntry.setOwner(publicKeys.at(ownerIndex));
			}

			for (auto i = 0u; i < publicKeys.size(); ++i)
				if (i != ownerIndex) {
					driveEntry.replicators().insert(publicKeys.at(i));
					driveEntry.replicatorInfos().emplace(publicKeys.at(i), state::ReplicatorInfo{test::Random(), test::Random()});
				}
		}

        void AssertValidationResult(
                ValidationResult expectedResult,
				const cache::CatapultCache& cache,
				const Key& signer,
				const Key& driveKey,
				const Hash256& dataModificationId,
				const std::vector<Key>& publicKeys) {
            // Arrange:
			const auto pOpinions = std::make_unique<uint64_t>();
			const auto pPublicKeys = std::unique_ptr<Key[]>(new Key[publicKeys.size()]);
			for (auto i = 0u; i < publicKeys.size(); ++i)
				memcpy(static_cast<void*>(&pPublicKeys[i]), static_cast<const void*>(&publicKeys.at(i)), sizeof(Key));

            Notification notification(
					signer,
					driveKey,
					dataModificationId,
					Used_Drive_Size,
					publicKeys.size(),
					pPublicKeys.get(),
					pOpinions.get()
			);
            auto pValidator = CreateDataModificationSingleApprovalValidator();
            
            // Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

            // Assert:
            EXPECT_EQ(expectedResult, result);
        }
    }

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		const auto signer = test::GenerateRandomByteArray<Key>();
		const auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
		const auto ownerIndex = test::RandomInRange(-1, Public_Keys_Count-1);	// -1 means that publicKeys don't contain drive owner key
		const auto publicKeys = test::GenerateUniqueRandomDataVector<Key>(Public_Keys_Count);
		PrepareDriveEntry(driveEntry, signer, dataModificationId, ownerIndex, publicKeys);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResult(
				ValidationResult::Success,
				cache,
				signer,
				driveEntry.key(),
				dataModificationId,
				publicKeys);
	}

    TEST(TEST_CLASS, FailureWhenDriveNotFound) {
    	// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto driveEntry = test::CreateBcDriveEntry();
		// No need to create drive delta.

		const auto signer = test::GenerateRandomByteArray<Key>();
		const auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
		const auto publicKeys = test::GenerateUniqueRandomDataVector<Key>(Public_Keys_Count);

		// Not inserting driveEntry into BcDriveCache.

		// Assert:
		AssertValidationResult(
				Failure_Storage_Drive_Not_Found,
				cache,
				signer,
				driveEntry.key(),
				dataModificationId,
				publicKeys);
    }

    TEST(TEST_CLASS, FailureWhenInvalidTransactionSigner) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		const auto signer = test::GenerateRandomByteArray<Key>();
		const auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
		const auto ownerIndex = test::RandomInRange(-1, Public_Keys_Count-1);
		const auto publicKeys = test::GenerateUniqueRandomDataVector<Key>(Public_Keys_Count);
		PrepareDriveEntry(driveEntry, signer, dataModificationId, ownerIndex, publicKeys);

		// Act:
		driveEntry.replicators().erase(signer);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResult(
				Failure_Storage_Invalid_Transaction_Signer,
				cache,
				signer,
				driveEntry.key(),
				dataModificationId,
				publicKeys);
    }

    TEST(TEST_CLASS, FailureWhenReocurringKeys) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		const auto signer = test::GenerateRandomByteArray<Key>();
		const auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
		const auto ownerIndex = test::RandomInRange(-1, Public_Keys_Count-1);
		auto publicKeys = test::GenerateUniqueRandomDataVector<Key>(Public_Keys_Count);
		PrepareDriveEntry(driveEntry, signer, dataModificationId, ownerIndex, publicKeys);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Act:
		const auto sourceKeyIndex = test::RandomInRange<uint8_t>(0, Public_Keys_Count-1);
		auto targetKeyIndex = test::RandomInRange<uint8_t>(0, Public_Keys_Count-2);
		targetKeyIndex += targetKeyIndex >= sourceKeyIndex;	// Compensate for the source key index
		publicKeys.at(targetKeyIndex) = publicKeys.at(sourceKeyIndex);

		// Assert:
		AssertValidationResult(
				Failure_Storage_Opinion_Reocurring_Keys,
				cache,
				signer,
				driveEntry.key(),
				dataModificationId,
				publicKeys);
    }

    TEST(TEST_CLASS, FailureWhenOpinionProvidedOnSelf) {
    	// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		const auto signer = test::GenerateRandomByteArray<Key>();
		const auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
		const auto ownerIndex = test::RandomInRange(-1, Public_Keys_Count-1);
		auto publicKeys = test::GenerateUniqueRandomDataVector<Key>(Public_Keys_Count);
		PrepareDriveEntry(driveEntry, signer, dataModificationId, ownerIndex, publicKeys);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Act:
		const auto targetKeyIndex = test::RandomInRange<uint8_t>(0, Public_Keys_Count-1);
		publicKeys.at(targetKeyIndex) = signer;

		// Assert:
		AssertValidationResult(
				Failure_Storage_Opinion_Provided_On_Self,
				cache,
				signer,
				driveEntry.key(),
				dataModificationId,
				publicKeys);
    }

    TEST(TEST_CLASS, FailureWhenInvalidKey) {
    	// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		const auto signer = test::GenerateRandomByteArray<Key>();
		const auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
		const auto ownerIndex = test::RandomInRange(-1, Public_Keys_Count-1);
		auto publicKeys = test::GenerateUniqueRandomDataVector<Key>(Public_Keys_Count);
		PrepareDriveEntry(driveEntry, signer, dataModificationId, ownerIndex, publicKeys);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Act:
		const auto targetKeyIndex = test::RandomInRange<uint8_t>(0, Public_Keys_Count-1);
		publicKeys.at(targetKeyIndex) = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
				Failure_Storage_Opinion_Invalid_Key,
				cache,
				signer,
				driveEntry.key(),
				dataModificationId,
				publicKeys);
    }

    TEST(TEST_CLASS, FailureWhenNoApprovedDataModifications) {
    	// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		const auto signer = test::GenerateRandomByteArray<Key>();
		const auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
		const auto ownerIndex = test::RandomInRange(-1, Public_Keys_Count-1);
		const auto publicKeys = test::GenerateUniqueRandomDataVector<Key>(Public_Keys_Count);
		PrepareDriveEntry(driveEntry, signer, dataModificationId, ownerIndex, publicKeys);

		// Act:
		driveEntry.completedDataModifications().clear();

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResult(
				Failure_Storage_No_Approved_Data_Modifications,
				cache,
				signer,
				driveEntry.key(),
				dataModificationId,
				publicKeys);
    }

    TEST(TEST_CLASS, FailureWhenInvalidDataModificationId) {
    	// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		const auto signer = test::GenerateRandomByteArray<Key>();
		const auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
		const auto ownerIndex = test::RandomInRange(-1, Public_Keys_Count-1);
		const auto publicKeys = test::GenerateUniqueRandomDataVector<Key>(Public_Keys_Count);
		PrepareDriveEntry(driveEntry, signer, dataModificationId, ownerIndex, publicKeys);

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResult(
				Failure_Storage_Invalid_Data_Modification_Id,
				cache,
				signer,
				driveEntry.key(),
				test::GenerateRandomByteArray<Hash256>(),	// Pass random hash as a DataModificationId.
				publicKeys);
    }

    // TODO: Double-check
    TEST(TEST_CLASS, FailureWhenEmptyReplicatorInfos) {
    	// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& driveDelta = delta.sub<cache::BcDriveCache>();
		auto driveEntry = test::CreateBcDriveEntry();

		const auto signer = test::GenerateRandomByteArray<Key>();
		const auto dataModificationId = test::GenerateRandomByteArray<Hash256>();
		const auto ownerIndex = test::RandomInRange(-1, Public_Keys_Count-1);
		const auto publicKeys = test::GenerateUniqueRandomDataVector<Key>(Public_Keys_Count);
		PrepareDriveEntry(driveEntry, signer, dataModificationId, ownerIndex, publicKeys);

		// Act:
		driveEntry.replicatorInfos().clear();

		driveDelta.insert(driveEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResult(
				Failure_Storage_No_Active_Data_Modifications,
				cache,
				signer,
				driveEntry.key(),
				dataModificationId,
				publicKeys);
    }
}}