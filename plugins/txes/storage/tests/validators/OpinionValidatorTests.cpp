/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/crypto/Signer.h"
#include "src/model/DownloadApprovalTransaction.h"
#include "src/validators/Validators.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS OpinionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Opinion, )

	namespace {
		using Notification = model::OpinionNotification<1>;

		constexpr auto Replicator_Count = 5;
		constexpr auto Common_Data_Size_Download_Approval = sizeof(Hash256) + sizeof(uint16_t) + sizeof(bool);

		/// Creates RawBuffer with given \a size and fills it with random data. Data pointer must be manually deleted after use.
		RawBuffer GenerateCommonDataBuffer(const size_t& size) {
			auto* const pCommonData = new uint8_t[size];
			MutableRawBuffer mutableBuffer(pCommonData, size);
			test::FillWithRandomData(mutableBuffer);
			return RawBuffer(mutableBuffer.pData, mutableBuffer.Size);
		}

		template<typename TOpinion>
		void AssertValidationResult(
				const ValidationResult& expectedResult,
				const cache::CatapultCache& cache,
				const RawBuffer& commonDataBuffer,
				const test::OpinionData<TOpinion>& opinionData) {
			// Arrange:
			const auto presentOpinionElementCount = opinionData.OpinionCount * opinionData.JudgedCount;
			const auto presentOpinionByteCount = (presentOpinionElementCount + 7) / 8;
			const auto payloadSize = opinionData.JudgedCount * sizeof(Key)
									 + opinionData.JudgingCount * sizeof(uint8_t)
									 + opinionData.OpinionCount * sizeof(BLSSignature)
									 + presentOpinionByteCount * sizeof(uint8_t)
									 + opinionData.OpinionElementCount * sizeof(TOpinion);
			const auto pPayload = std::unique_ptr<uint8_t[]>(new uint8_t[payloadSize]);

			auto* const pPublicKeysBegin = reinterpret_cast<Key*>(pPayload.get());
			for (auto i = 0u; i < opinionData.PublicKeys.size(); ++i)
				memcpy(static_cast<void*>(&pPublicKeysBegin[i]), static_cast<const void*>(&opinionData.PublicKeys.at(i)), sizeof(Key));

			auto* const pOpinionIndicesBegin = reinterpret_cast<uint8_t*>(pPublicKeysBegin + opinionData.PublicKeys.size());
			for (auto i = 0u; i < opinionData.OpinionIndices.size(); ++i)
				memcpy(static_cast<void*>(&pOpinionIndicesBegin[i]), static_cast<const void*>(&opinionData.OpinionIndices.at(i)), sizeof(uint8_t));

			auto* const pBlsSignaturesBegin = reinterpret_cast<BLSSignature*>(pOpinionIndicesBegin + opinionData.OpinionIndices.size());
			for (auto i = 0u; i < opinionData.BlsSignatures.size(); ++i)
				memcpy(static_cast<void*>(&pBlsSignaturesBegin[i]), static_cast<const void*>(&opinionData.BlsSignatures.at(i)), sizeof(BLSSignature));

			auto* const pPresentOpinionsBegin = reinterpret_cast<uint8_t*>(pBlsSignaturesBegin + opinionData.BlsSignatures.size());
			for (auto i = 0u; i < presentOpinionByteCount; ++i) {
				boost::dynamic_bitset<uint8_t> byte(8, 0u);
				for (auto j = 0u; j < std::min(8u, presentOpinionElementCount - i*8); ++j) {
					const auto bitNumber = i*8 + j;
					byte[j] = opinionData.PresentOpinions.at(bitNumber / opinionData.JudgedCount).at(bitNumber % opinionData.JudgedCount);
				}
				boost::to_block_range(byte, &pPresentOpinionsBegin[i]);
			}

			auto* const pOpinionsBegin = reinterpret_cast<TOpinion*>(pPresentOpinionsBegin + presentOpinionByteCount);
			auto i = 0u;
			for (const auto& opinion : opinionData.Opinions)
				for (const auto& opinionElement : opinion)
					memcpy(static_cast<void*>(&pOpinionsBegin[i++]), static_cast<const void*>(&opinionElement), sizeof(TOpinion));

			Notification notification(
					commonDataBuffer.Size,
					opinionData.OpinionCount,
					opinionData.JudgingCount,
					opinionData.JudgedCount,
					commonDataBuffer.pData,
					pPublicKeysBegin,
					pOpinionIndicesBegin,
					pBlsSignaturesBegin,
					pPresentOpinionsBegin,
					pOpinionsBegin
			);
			auto pValidator = CreateOpinionValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);
			delete[] commonDataBuffer.pData;

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer);

		// Assert:
		AssertValidationResult(
				ValidationResult::Success,
				cache,
				commonDataBuffer,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenUnusedKeys) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer);

		// Act:
		const auto keyIndex = test::RandomInRange(0, opinionData.JudgedCount-1);	// Select random key
		for (auto i = 0; i < opinionData.OpinionCount; ++i)
			opinionData.PresentOpinions.at(i).at(keyIndex) = false;	// Reset all bits in corresponding column in PresentOpinions

		// Assert:
		AssertValidationResult(
				Failure_Storage_Opinion_Unused_Key,
				cache,
				commonDataBuffer,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenReoccuringKeys) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		const auto judgedCount = test::RandomInRange<uint8_t>(2, replicatorKeyPairs.size());	// Need at least 2 different keys
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, judgedCount);

		// Act:
		const auto sourceKeyIndex = test::RandomInRange(0, opinionData.JudgedCount-1);	// Select random key as a source
		auto targetKeyIndex = test::RandomInRange(0, opinionData.JudgedCount-2);	// Select random key from remaining keys as a target
		targetKeyIndex += targetKeyIndex >= sourceKeyIndex;	// Compensate for the source key
		opinionData.PublicKeys.at(targetKeyIndex) = opinionData.PublicKeys.at(sourceKeyIndex);	// Replace target key with a source key

		// Assert:
		AssertValidationResult(
				Failure_Storage_Opinion_Reocurring_Keys,
				cache,
				commonDataBuffer,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenReplicatorNotFound) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer);

		// Act:
		const auto replicatorIndex = test::RandomInRange(0, opinionData.JudgingCount-1);	// Select random replicator
		auto delta = cache.createDelta();
		auto& replicatorDelta = delta.sub<cache::ReplicatorCache>();
		replicatorDelta.remove(opinionData.PublicKeys.at(replicatorIndex));	// Remove selected replicator from ReplicatorCache
		cache.commit(cache.height());	// Commit removal

		// Assert:
		AssertValidationResult(
				Failure_Storage_Replicator_Not_Found,
				cache,
				commonDataBuffer,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenInvalidOpinionIndex) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer);

		// Act:
		const auto positionIndex = test::RandomInRange(0, opinionData.JudgingCount-1);	// Select random position in OpinionIndices
		opinionData.OpinionIndices.at(positionIndex) += opinionData.OpinionCount;	// Make stored index invalid (valid indices must be less than OpinionCount)

		// Assert:
		AssertValidationResult(
				Failure_Storage_Invalid_Opinion_Index,
				cache,
				commonDataBuffer,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenReocurringIndividualParts) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);

		const auto maxCount = replicatorKeyPairs.size();
		const auto opinionCount = test::RandomInRange<uint8_t>(2, maxCount);	// Need at least 2 different opinions
		const auto judgingCount = test::RandomInRange<uint8_t>(opinionCount, maxCount);	// Generating the rest of the counts
		const auto judgedCount = test::RandomInRange<uint8_t>(judgingCount, maxCount);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, judgedCount, judgingCount, opinionCount, true);

		// Act:
		auto sourceOpinionIndex = test::RandomInRange(0, opinionData.OpinionCount-2);	// Select random opinion as a source; it can't be the last opinion
		auto targetOpinionIndex = test::RandomInRange(sourceOpinionIndex+1, opinionData.OpinionCount-1);	// Select random opinion from following opinions as a target
		targetOpinionIndex += targetOpinionIndex == opinionData.FilledPresenceRowIndex;	// Compensate for the fully present opinion. Safe since this opinion is guaranteed not to be the last
		opinionData.OpinionElementCount += opinionData.Opinions.at(sourceOpinionIndex).size() - opinionData.Opinions.at(targetOpinionIndex).size();	// Update OpinionElementCount
		opinionData.Opinions.at(targetOpinionIndex) = opinionData.Opinions.at(sourceOpinionIndex);	// Replace target opinion with a source opinion
		opinionData.PresentOpinions.at(targetOpinionIndex) = opinionData.PresentOpinions.at(sourceOpinionIndex);	// Replace target opinion row in PresentOpinions with a source opinion row

		// Assert:
		AssertValidationResult(
				Failure_Storage_Opinions_Reocurring_Individual_Parts,
				cache,
				commonDataBuffer,
				opinionData);
	}

	TEST(TEST_CLASS, FailureWhenInvalidBlsSignature) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer);

		// Act:
		const auto signatureIndex = test::RandomInRange(0, opinionData.OpinionCount-1);	// Select random signature
		opinionData.BlsSignatures.at(signatureIndex) = test::GenerateRandomByteArray<BLSSignature>();	// Replace selected signature with random data

		// Assert:
		AssertValidationResult(
				Failure_Storage_Invalid_BLS_Signature,
				cache,
				commonDataBuffer,
				opinionData);
	}
}}