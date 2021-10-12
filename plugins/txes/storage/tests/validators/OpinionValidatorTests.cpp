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
			const auto totalJudgedKeysCount = opinionData.OverlappingKeysCount + opinionData.JudgedKeysCount;
			const auto presentOpinionElementCount = opinionData.OpinionCount * totalJudgedKeysCount;
			const auto presentOpinionByteCount = (presentOpinionElementCount + 7) / 8;
			const auto payloadSize = opinionData.PublicKeys.size() * sizeof(Key)
									 + opinionData.OpinionIndices.size() * sizeof(uint8_t)
									 + opinionData.BlsSignatures.size() * sizeof(BLSSignature)
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
					byte[j] = opinionData.PresentOpinions.at(bitNumber / totalJudgedKeysCount).at(bitNumber % totalJudgedKeysCount);
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
					opinionData.JudgingKeysCount,
					opinionData.OverlappingKeysCount,
					opinionData.JudgedKeysCount,
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
		const auto totalJudgedKeysCount = opinionData.OverlappingKeysCount + opinionData.JudgedKeysCount;
		const auto keyIndex = test::RandomInRange(0, totalJudgedKeysCount-1);	// Select random judged key
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

		const auto totalKeysCount = test::RandomInRange(2ul, replicatorKeyPairs.size());	// Need at least 2 different keys
		const auto overlappingKeysCount = test::RandomInRange<uint8_t>(totalKeysCount > 1 ? 0 : 1, totalKeysCount);
		const auto judgingKeysCount = test::RandomInRange<uint8_t>(overlappingKeysCount ? 0 : 1, totalKeysCount - overlappingKeysCount - (overlappingKeysCount ? 0 : 1));
		const auto judgedKeysCount = totalKeysCount - overlappingKeysCount - judgingKeysCount;
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {judgingKeysCount, overlappingKeysCount, judgedKeysCount});

		// Act:
		const auto sourceKeyIndex = test::RandomInRange(0ul, totalKeysCount-1);	// Select random key as a source
		auto targetKeyIndex = test::RandomInRange(0ul, totalKeysCount-2);	// Select random key from remaining keys as a target
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
		const auto totalJudgingKeysCount = opinionData.JudgingKeysCount + opinionData.OverlappingKeysCount;
		const auto replicatorIndex = test::RandomInRange(0, totalJudgingKeysCount-1);	// Select random judging replicator
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
		const auto positionIndex = test::RandomInRange(0ul, opinionData.OpinionIndices.size()-1);	// Select random position in OpinionIndices
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

		const auto totalKeysCount = test::RandomInRange(2ul, replicatorKeyPairs.size());
		const auto opinionCount = test::RandomInRange<uint8_t>(2, totalKeysCount);	// Need at least 2 different opinions
		const auto totalJudgingKeysCount = test::RandomInRange<uint8_t>(opinionCount, totalKeysCount);
		const auto judgedKeysCount = totalKeysCount - totalJudgingKeysCount;
		const auto overlappingKeysCount = test::RandomInRange<uint8_t>(judgedKeysCount ? 0 : 1, totalJudgingKeysCount);
		const auto judgingKeysCount = totalJudgingKeysCount - overlappingKeysCount;
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer, {judgingKeysCount, overlappingKeysCount, judgedKeysCount}, opinionCount, true);

		// Act:
		auto sourceOpinionIndex = test::RandomInRange(0ul, opinionData.Opinions.size()-2);	// Select random opinion as a source; it can't be the last opinion
		auto targetOpinionIndex = test::RandomInRange(sourceOpinionIndex+1, opinionData.Opinions.size()-1);	// Select random opinion from following opinions as a target
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
		const auto signatureIndex = test::RandomInRange(0ul, opinionData.BlsSignatures.size()-1);	// Select random signature
		opinionData.BlsSignatures.at(signatureIndex) = test::GenerateRandomByteArray<BLSSignature>();	// Replace selected signature with random data

		// Assert:
		AssertValidationResult(
				Failure_Storage_Invalid_BLS_Signature,
				cache,
				commonDataBuffer,
				opinionData);
	}
}}