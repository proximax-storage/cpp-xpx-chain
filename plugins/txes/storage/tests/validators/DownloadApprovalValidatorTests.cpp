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
		constexpr auto Replicator_Count = 5;
		constexpr auto Common_Data_Size_Download_Approval = sizeof(Hash256) + sizeof(uint16_t) + sizeof(bool);

		void AssertValidationResult(
				const ValidationResult& expectedResult,
				const cache::CatapultCache& cache,
				const Hash256& downloadChannelId,
				const uint16_t sequenceNumber,
				const test::OpinionData<uint64_t>& opinionData) {
			// Arrange:
			const auto totalJudgedKeysCount = opinionData.OverlappingKeysCount + opinionData.JudgedKeysCount;
			const auto presentOpinionElementCount = opinionData.OpinionCount * totalJudgedKeysCount;
			const auto presentOpinionByteCount = (presentOpinionElementCount + 7) / 8;

			const auto pOpinionIndicesBegin = std::unique_ptr<uint8_t[]>(new uint8_t[opinionData.OpinionIndices.size()]);
			for (auto i = 0u; i < opinionData.OpinionIndices.size(); ++i)
				memcpy(static_cast<void*>(&pOpinionIndicesBegin[i]), static_cast<const void*>(&opinionData.OpinionIndices.at(i)), sizeof(uint8_t));
			const auto pPresentOpinionsBegin = std::unique_ptr<uint8_t[]>(new uint8_t[presentOpinionByteCount]);
			for (auto i = 0u; i < presentOpinionByteCount; ++i) {
				boost::dynamic_bitset<uint8_t> byte(8, 0u);
				for (auto j = 0u; j < std::min(8u, presentOpinionElementCount - i*8); ++j) {
					const auto bitNumber = i*8 + j;
					byte[j] = opinionData.PresentOpinions.at(bitNumber / totalJudgedKeysCount).at(bitNumber % totalJudgedKeysCount);
				}
				boost::to_block_range(byte, &pPresentOpinionsBegin[i]);
			}

			Notification notification(
					downloadChannelId,
					sequenceNumber,
					opinionData.OpinionCount,
					opinionData.JudgingKeysCount,
					opinionData.OverlappingKeysCount,
					opinionData.JudgedKeysCount,
					pOpinionIndicesBegin.get(),
					pPresentOpinionsBegin.get()
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
				const Hash256& downloadChannelId,
				const uint16_t sequenceNumber) {
			// Generate valid opinion data:
			std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
			replicatorKeyPairs.reserve(Replicator_Count);
			for (auto i = replicatorKeyPairs.size(); i < Replicator_Count; ++i)	// No need to actually create replicator entries, since they are not used in DownloadApprovalValidator.
				replicatorKeyPairs.emplace_back(
						test::GenerateRandomByteArray<Key>(),
						crypto::BLSKeyPair::FromPrivate(crypto::BLSPrivateKey::Generate(test::RandomByte)));

			const auto totalKeysCount = test::RandomInRange<uint8_t>(1, replicatorKeyPairs.size());
			const auto overlappingKeysCount = test::RandomInRange<uint8_t>(1, totalKeysCount);
			const auto judgedKeysCount = totalKeysCount - overlappingKeysCount;
			const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
			auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {0, overlappingKeysCount, judgedKeysCount});
			delete[] commonDataBuffer.pData;

			std::vector<std::vector<uint8_t>> publicKeysIndices(opinionData.OpinionCount);	// Nth vector in publicKeysIndices contains indices of all public keys of replicators that provided Nth opinion.
			for (auto i = 0; i < overlappingKeysCount; ++i)
				publicKeysIndices.at(opinionData.OpinionIndices.at(i)).push_back(i);
			for (auto i = 0; i < opinionData.OpinionCount; ++i)
				for (const auto index : publicKeysIndices.at(i))
					opinionData.PresentOpinions.at(i).at(index) = true;

			// Pass:
			AssertValidationResult(
					expectedResult,
					cache,
					downloadChannelId,
					sequenceNumber,
					opinionData);
		}
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto downloadChannelEntry = test::CreateDownloadChannelEntry();
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				ValidationResult::Success,
				cache,
				downloadChannelEntry.id(),
				downloadChannelEntry.downloadApprovalCount()+1);
	}

	TEST(TEST_CLASS, FailureWhenDownloadChannelNotFound) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto downloadChannelEntry = test::CreateDownloadChannelEntry();
		// Not inserting downloadChannelEntry into DownloadChannelCache.

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Download_Channel_Not_Found,
				cache,
				downloadChannelEntry.id(),
				downloadChannelEntry.downloadApprovalCount()+1);
	}

	TEST(TEST_CLASS, FailureWhenTransactionAlreadyApproved) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto downloadChannelEntry = test::CreateDownloadChannelEntry();
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Transaction_Already_Approved,
				cache,
				downloadChannelEntry.id(),
				downloadChannelEntry.downloadApprovalCount());
	}

	TEST(TEST_CLASS, FailureWhenSequenceNumberLowerThanExpected) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto downloadChannelEntry = test::CreateDownloadChannelEntry();
		if (downloadChannelEntry.downloadApprovalCount() == 0)	// Overflow prevention
			downloadChannelEntry.incrementDownloadApprovalCount();
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Invalid_Sequence_Number,
				cache,
				downloadChannelEntry.id(),
				test::RandomInRange<uint16_t>(0, downloadChannelEntry.downloadApprovalCount() - 1));
	}

	TEST(TEST_CLASS, FailureWhenSequenceNumberGreaterThanExpected) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto downloadChannelEntry = test::CreateDownloadChannelEntry();
		const auto upperLimit = std::numeric_limits<uint16_t>::max();
		if (downloadChannelEntry.downloadApprovalCount() > upperLimit - 2)	// Overflow prevention
			downloadChannelEntry.setDownloadApprovalCount(upperLimit - 2);
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResultWithoutOpinionData(
				Failure_Storage_Invalid_Sequence_Number,
				cache,
				downloadChannelEntry.id(),
				test::RandomInRange<uint16_t>(downloadChannelEntry.downloadApprovalCount() + 2, upperLimit));
	}

	TEST(TEST_CLASS, FailureWhenHaveJudgingReplicators) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto downloadChannelEntry = test::CreateDownloadChannelEntry();
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		replicatorKeyPairs.reserve(Replicator_Count);
		for (auto i = replicatorKeyPairs.size(); i < Replicator_Count; ++i)	// No need to actually create replicator entries, since they are not used in DownloadApprovalValidator.
			replicatorKeyPairs.emplace_back(
					test::GenerateRandomByteArray<Key>(),
					crypto::BLSKeyPair::FromPrivate(crypto::BLSPrivateKey::Generate(test::RandomByte)));

		const auto totalKeysCount = test::RandomInRange<uint8_t>(2, replicatorKeyPairs.size());
		const auto judgingKeysCount = test::RandomInRange<uint8_t>(1, totalKeysCount-1);
		const auto overlappingKeysCount = test::RandomInRange<uint8_t>(0, totalKeysCount - judgingKeysCount);
		const auto judgedKeysCount = totalKeysCount - judgingKeysCount - overlappingKeysCount;
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {judgingKeysCount, overlappingKeysCount, judgedKeysCount});
		delete[] commonDataBuffer.pData;

		// Assert:
		AssertValidationResult(
				Failure_Storage_No_Opinion_Provided_On_Self,
				cache,
				downloadChannelEntry.id(),
				downloadChannelEntry.downloadApprovalCount()+1,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenNoOpinionProvidedOnSelf) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		auto delta = cache.createDelta();
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto downloadChannelEntry = test::CreateDownloadChannelEntry();
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		replicatorKeyPairs.reserve(Replicator_Count);
		for (auto i = replicatorKeyPairs.size(); i < Replicator_Count; ++i)	// No need to actually create replicator entries, since they are not used in DownloadApprovalValidator.
			replicatorKeyPairs.emplace_back(
					test::GenerateRandomByteArray<Key>(),
					crypto::BLSKeyPair::FromPrivate(crypto::BLSPrivateKey::Generate(test::RandomByte)));

		const auto totalKeysCount = test::RandomInRange<uint8_t>(1, replicatorKeyPairs.size());
		const auto overlappingKeysCount = test::RandomInRange<uint8_t>(1, totalKeysCount);
		const auto judgedKeysCount = totalKeysCount - overlappingKeysCount;
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {0, overlappingKeysCount, judgedKeysCount});
		delete[] commonDataBuffer.pData;

		std::vector<std::vector<uint8_t>> publicKeysIndices(opinionData.OpinionCount);	// Nth vector in publicKeysIndices contains indices of all public keys of replicators that provided Nth opinion.
		for (auto i = 0; i < overlappingKeysCount; ++i)
			publicKeysIndices.at(opinionData.OpinionIndices.at(i)).push_back(i);

		// Act:
		const auto targetKeyIndex = test::RandomInRange(0, overlappingKeysCount-1);	// Select a random key of judging replicator that won't provide an opinion on itself.
		for (auto i = 0; i < opinionData.OpinionCount; ++i)
			for (const auto index : publicKeysIndices.at(i))
				opinionData.PresentOpinions.at(i).at(index) = index != targetKeyIndex;

		// Assert:
		AssertValidationResult(
				Failure_Storage_No_Opinion_Provided_On_Self,
				cache,
				downloadChannelEntry.id(),
				downloadChannelEntry.downloadApprovalCount()+1,
				opinionData);
	}
}}
