/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/crypto/Signer.h"
#include "src/validators/Validators.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DownloadApprovalValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(DownloadApproval, )

	namespace {
		using Notification = model::DownloadApprovalNotification<1>;

		constexpr auto Current_Height = Height(10);
		constexpr auto Shard_Size = 5;
		constexpr auto Required_Signatures_Count = Shard_Size * 2 / 3 + 1;
		constexpr auto Common_Data_Size_Download_Approval = sizeof(Hash256) + sizeof(Hash256) + sizeof(uint16_t) + sizeof(bool);

		std::vector<crypto::KeyPair> CreateReplicatorKeyPairs(const uint16_t& replicatorCount = Shard_Size) {
			std::vector<crypto::KeyPair> replicatorKeyPairs;
			test::PopulateReplicatorKeyPairs(replicatorKeyPairs, replicatorCount);
			return replicatorKeyPairs;
		};

		state::DownloadChannelEntry CreateDownloadChannelEntry(const std::vector<crypto::KeyPair>& replicatorKeyPairs, const std::optional<Hash256>& event) {
			auto entry = test::CreateDownloadChannelEntry();
			entry.cumulativePayments().clear();
			for (const auto& keyPair : replicatorKeyPairs) {
				entry.shardReplicators().insert(keyPair.publicKey());
				entry.cumulativePayments().emplace(keyPair.publicKey(), Amount(0));
			}

			entry.downloadApprovalInitiationEvent() = event;

			return entry;
		}

		void AssertValidationResult(
				const ValidationResult& expectedResult,
				const cache::CatapultCache& cache,
				const Hash256& downloadChannelId,
				const Hash256& event,
				const uint16_t sequenceNumber,
				const test::OpinionData<uint64_t>& opinionData) {
			// Arrange:
			const auto totalKeysCount = opinionData.JudgingKeysCount + opinionData.OverlappingKeysCount + opinionData.JudgedKeysCount;
			const auto totalJudgingKeysCount = totalKeysCount - opinionData.JudgedKeysCount;
			const auto totalJudgedKeysCount = totalKeysCount - opinionData.JudgingKeysCount;
			const auto presentOpinionElementCount = totalJudgingKeysCount * totalJudgedKeysCount;
			const auto presentOpinionByteCount = (presentOpinionElementCount + 7) / 8;

			const auto pPublicKeys = std::make_unique<Key[]>(totalKeysCount);
			for (auto i = 0u; i < totalKeysCount; ++i)
				pPublicKeys[i] = opinionData.PublicKeys.at(i);

			const auto pPresentOpinions = std::make_unique<uint8_t[]>(presentOpinionByteCount);
			for (auto i = 0u; i < presentOpinionByteCount; ++i) {
				boost::dynamic_bitset<uint8_t> byte(8, 0u);
				for (auto j = 0u; j < std::min(8u, presentOpinionElementCount - i*8); ++j) {
					const auto bitNumber = i*8 + j;
					byte[j] = opinionData.PresentOpinions.at(bitNumber / totalJudgedKeysCount).at(bitNumber % totalJudgedKeysCount);
				}
				boost::to_block_range(byte, &pPresentOpinions[i]);
			}

			std::vector<Key> replicators;
			replicators.reserve(totalKeysCount);
			for (auto i = 0u; i < totalKeysCount; ++i)
				replicators.emplace_back(test::GenerateRandomByteArray<Key>());

			Notification notification(
					downloadChannelId,
					event,
					opinionData.JudgingKeysCount,
					opinionData.OverlappingKeysCount,
					opinionData.JudgedKeysCount,
					pPublicKeys.get(),
					pPresentOpinions.get()
			);
			auto pValidator = CreateDownloadApprovalValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		void AssertValidationResultWithoutOpinionData(
				const ValidationResult& expectedResult,
				cache::CatapultCache& cache,
				const std::vector<crypto::KeyPair>& replicatorKeyPairs,
				const Hash256& downloadChannelId,
				const Hash256& eventHash,
				const uint16_t sequenceNumber) {
			// Generate valid opinion data:
			const auto totalKeysCount = test::RandomInRange<uint8_t>(Required_Signatures_Count, Shard_Size);
			const auto overlappingKeysCount = test::RandomInRange<uint8_t>(Required_Signatures_Count, totalKeysCount);
			const auto judgedKeysCount = totalKeysCount - overlappingKeysCount;
			const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
			auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {0, overlappingKeysCount, judgedKeysCount});
			delete[] commonDataBuffer.pData;

			// Make sure that every judging replicator has provided an opinion on self:
			for (auto i = 0; i < overlappingKeysCount; ++i)
				opinionData.PresentOpinions.at(i).at(i) = true;

			// Pass:
			AssertValidationResult(
					expectedResult,
					cache,
					downloadChannelId,
					eventHash,
					sequenceNumber,
					opinionData);
		}
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto replicatorKeyPairs = CreateReplicatorKeyPairs();
		auto eventHash = test::GenerateRandomByteArray<Hash256>();
		auto downloadChannelEntry = CreateDownloadChannelEntry(replicatorKeyPairs, eventHash);
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				ValidationResult::Success,
				cache,
				replicatorKeyPairs,
				downloadChannelEntry.id(),
				eventHash,
				1); // downloadChannelEntry.downloadApprovalCount()+1);
	}

	TEST(TEST_CLASS, FailureWhenDownloadChannelNotFound) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto replicatorKeyPairs = CreateReplicatorKeyPairs();
		auto eventHash = test::GenerateRandomByteArray<Hash256>();
		auto downloadChannelEntry = CreateDownloadChannelEntry(replicatorKeyPairs, eventHash);
		// Not inserting downloadChannelEntry into DownloadChannelCache.

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Download_Channel_Not_Found,
				cache,
				replicatorKeyPairs,
				downloadChannelEntry.id(),
				eventHash,
				1);// downloadChannelEntry.downloadApprovalCount()+1);
	}

	TEST(TEST_CLASS, FailureWhenSignatureCountInsufficient) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto replicatorKeyPairs = CreateReplicatorKeyPairs();
		auto eventHash = test::GenerateRandomByteArray<Hash256>();
		auto downloadChannelEntry = CreateDownloadChannelEntry(replicatorKeyPairs, eventHash);

		// Double the number of replicators in downloadChannelEntry's shard
		for (auto i = 0u; i < Shard_Size; ++i)
			downloadChannelEntry.shardReplicators().insert(test::GenerateRandomByteArray<Key>());

		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Signature_Count_Insufficient,
				cache,
				replicatorKeyPairs,
				downloadChannelEntry.id(),
				eventHash,
				1);// downloadChannelEntry.downloadApprovalCount()+1);
	}

	TEST(TEST_CLASS, FailureWhenTransactionAlreadyApproved) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto replicatorKeyPairs = CreateReplicatorKeyPairs();
		auto eventHash = test::GenerateRandomByteArray<Hash256>();
		auto downloadChannelEntry = CreateDownloadChannelEntry(replicatorKeyPairs, {});
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Invalid_Approval_Trigger,
				cache,
				replicatorKeyPairs,
				downloadChannelEntry.id(),
				eventHash,
				1); // downloadChannelEntry.downloadApprovalCount());
	}

	TEST(TEST_CLASS, FailureWhenInvalidApprovalTrigger) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto replicatorKeyPairs = CreateReplicatorKeyPairs();
		auto eventHash = test::GenerateRandomByteArray<Hash256>();
		auto downloadChannelEntry = CreateDownloadChannelEntry(replicatorKeyPairs, test::GenerateRandomByteArray<Hash256>());
//		if (downloadChannelEntry.downloadApprovalCount() == 0)	// Overflow prevention
//			downloadChannelEntry.incrementDownloadApprovalCount();
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Invalid_Approval_Trigger,
				cache,
				replicatorKeyPairs,
				downloadChannelEntry.id(),
				eventHash,
				1); // downloadChannelEntry.downloadApprovalCount() - 1));
	}

	TEST(TEST_CLASS, FailureWhenHaveJudgingReplicators) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto replicatorKeyPairs = CreateReplicatorKeyPairs();
		auto eventHash = test::GenerateRandomByteArray<Hash256>();
		auto downloadChannelEntry = CreateDownloadChannelEntry(replicatorKeyPairs, eventHash);
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		const auto totalKeysCount = test::RandomInRange<uint8_t>(Required_Signatures_Count, Shard_Size);
		const auto judgingKeysCount = test::RandomInRange<uint8_t>(1, totalKeysCount-1);
		const auto overlappingKeysCount = test::RandomInRange<uint8_t>(Required_Signatures_Count - judgingKeysCount, totalKeysCount - judgingKeysCount);
		const auto judgedKeysCount = totalKeysCount - judgingKeysCount - overlappingKeysCount;
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {judgingKeysCount, overlappingKeysCount, judgedKeysCount});
		delete[] commonDataBuffer.pData;

		// Assert:
		AssertValidationResult(
				Failure_Storage_No_Opinion_Provided_On_Self,
				cache,
				downloadChannelEntry.id(),
				eventHash,
				1,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenNoOpinionProvidedOnSelf) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto replicatorKeyPairs = CreateReplicatorKeyPairs();
		auto eventHash = test::GenerateRandomByteArray<Hash256>();
		auto downloadChannelEntry = CreateDownloadChannelEntry(replicatorKeyPairs, eventHash);
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		const auto totalKeysCount = test::RandomInRange<uint8_t>(Required_Signatures_Count, Shard_Size);
		const auto overlappingKeysCount = test::RandomInRange<uint8_t>(Required_Signatures_Count, totalKeysCount);
		const auto judgedKeysCount = totalKeysCount - overlappingKeysCount;
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {0, overlappingKeysCount, judgedKeysCount});
		delete[] commonDataBuffer.pData;

		// Act:
		const auto targetKeyIndex = test::RandomInRange(0, overlappingKeysCount-1);	// Select a random key of judging replicator that won't provide an opinion on itself.
		for (auto i = 0; i < overlappingKeysCount; ++i)
			opinionData.PresentOpinions.at(i).at(targetKeyIndex) = i != targetKeyIndex;

		// Assert:
		AssertValidationResult(
				Failure_Storage_No_Opinion_Provided_On_Self,
				cache,
				downloadChannelEntry.id(),
				eventHash,
				1,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenInvalidKey) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto replicatorKeyPairs = CreateReplicatorKeyPairs();
		auto eventHash = test::GenerateRandomByteArray<Hash256>();
		auto downloadChannelEntry = CreateDownloadChannelEntry(replicatorKeyPairs, eventHash);
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Act:
		const auto targetKeyIndex = test::RandomInRange(0, Required_Signatures_Count-1);
		replicatorKeyPairs.at(targetKeyIndex) = crypto::KeyPair::FromPrivate(crypto::PrivateKey::Generate(test::RandomByte));

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Opinion_Invalid_Key,
				cache,
				replicatorKeyPairs,
				downloadChannelEntry.id(),
				eventHash,
				1);
	}
}}
