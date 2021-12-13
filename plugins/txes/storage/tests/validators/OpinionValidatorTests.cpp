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

		template<typename TOpinion>
		void AssertValidationResult(
				const ValidationResult& expectedResult,
				const cache::CatapultCache& cache,
				const RawBuffer& commonDataBuffer,
				const test::OpinionData<TOpinion>& opinionData) {
			// Arrange:
			const auto totalJudgingKeysCount = opinionData.JudgingKeysCount + opinionData.OverlappingKeysCount;
			const auto totalJudgedKeysCount = opinionData.OverlappingKeysCount + opinionData.JudgedKeysCount;
			const auto presentOpinionElementCount = totalJudgingKeysCount * totalJudgedKeysCount;
			const auto presentOpinionByteCount = (presentOpinionElementCount + 7) / 8;
			const auto payloadSize = opinionData.PublicKeys.size() * sizeof(Key)
									 + opinionData.Signatures.size() * sizeof(Signature)
									 + presentOpinionByteCount * sizeof(uint8_t)
									 + opinionData.OpinionElementCount * sizeof(TOpinion);
			const auto pPayload = std::unique_ptr<uint8_t[]>(new uint8_t[payloadSize]);

			auto* const pPublicKeysBegin = reinterpret_cast<Key*>(pPayload.get());
			for (auto i = 0u; i < opinionData.PublicKeys.size(); ++i)
				memcpy(static_cast<void*>(&pPublicKeysBegin[i]), static_cast<const void*>(&opinionData.PublicKeys.at(i)), sizeof(Key));

			auto* const pSignaturesBegin = reinterpret_cast<Signature*>(pPublicKeysBegin + opinionData.PublicKeys.size());
			for (auto i = 0u; i < opinionData.Signatures.size(); ++i)
				memcpy(static_cast<void*>(&pSignaturesBegin[i]), static_cast<const void*>(&opinionData.Signatures.at(i)), sizeof(Signature));

			auto* const pPresentOpinionsBegin = reinterpret_cast<uint8_t*>(pSignaturesBegin + opinionData.Signatures.size());
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
					opinionData.JudgingKeysCount,
					opinionData.OverlappingKeysCount,
					opinionData.JudgedKeysCount,
					commonDataBuffer.pData,
					pPublicKeysBegin,
					pSignaturesBegin,
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
		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
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
		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer);

		// Act:
		const auto totalJudgingKeysCount = opinionData.JudgingKeysCount + opinionData.OverlappingKeysCount;
		const auto totalJudgedKeysCount = opinionData.OverlappingKeysCount + opinionData.JudgedKeysCount;
		const auto keyIndex = test::RandomInRange(0, totalJudgedKeysCount-1);	// Select random judged key
		for (auto i = 0; i < totalJudgingKeysCount; ++i)
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
		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);

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

	TEST(TEST_CLASS, FailureWhenInvalidSignature) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();
		std::vector<crypto::KeyPair> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorKeyPairs, Replicator_Count);
		const auto commonDataBuffer = test::GenerateCommonDataBuffer(Common_Data_Size_Download_Approval);
		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, commonDataBuffer);

		// Act:
		const auto signatureIndex = test::RandomInRange(0ul, opinionData.Signatures.size()-1);	// Select random signature
		opinionData.Signatures.at(signatureIndex) = test::GenerateRandomByteArray<Signature>();	// Replace selected signature with random data

		// Assert:
		AssertValidationResult(
				Failure_Storage_Opinion_Invalid_Signature,
				cache,
				commonDataBuffer,
				opinionData);
	}
}}