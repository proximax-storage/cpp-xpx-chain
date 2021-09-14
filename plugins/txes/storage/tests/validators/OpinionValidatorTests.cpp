/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/crypto/Signer.h"
#include "src/model/DownloadApprovalTransaction.h"
#include "src/utils/StorageUtils.h"
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
		constexpr auto Current_Height = Height(10);

		ValidationResult GetValidationResult(
				const model::DownloadApprovalTransaction& transaction,
				const cache::ReadOnlyCatapultCache& cache = test::StorageCacheFactory::Create().createView().toReadOnly(),
				const config::BlockchainConfiguration& config = config::BlockchainConfiguration::Uninitialized(),
				Height height = Height(1)) {
			const auto commonDataSize = sizeof(transaction.DownloadChannelId)
										+ sizeof(transaction.SequenceNumber)
										+ sizeof(transaction.ResponseToFinishDownloadTransaction);
			auto* const commonDataPtr = new uint8_t[commonDataSize];	// TODO: Make smart pointer
			{
				auto* pCommonData = commonDataPtr;
				utils::WriteToByteArray(pCommonData, transaction.DownloadChannelId);
				utils::WriteToByteArray(pCommonData, transaction.SequenceNumber);
				utils::WriteToByteArray(pCommonData, transaction.ResponseToFinishDownloadTransaction);
			}
			Notification notification(
					commonDataSize,
					transaction.OpinionCount,
					transaction.JudgingCount,
					transaction.JudgedCount,
					commonDataPtr,
					transaction.PublicKeysPtr(),
					transaction.OpinionIndicesPtr(),
					transaction.BlsSignaturesPtr(),
					transaction.PresentOpinionsPtr(),
					transaction.OpinionsPtr()
			);

			auto pValidator = CreateOpinionValidator();
			auto context = test::CreateValidatorContext(config, std::move(height), cache);
			auto result = test::ValidateNotification(*pValidator, notification, context);

			delete commonDataPtr;

			return result;
		}
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto cache = test::StorageCacheFactory::Create();

		const auto replicatorCount = 10;
		std::vector<std::pair<Key, crypto::BLSKeyPair>> replicatorKeyPairs;
		test::AddReplicators(cache, replicatorCount, replicatorKeyPairs);

		const auto downloadChannelId = test::GenerateRandomByteArray<Hash256>();
		const auto sequenceNumber = test::Random16();
		const bool response = !(test::Random() % 5); // True with probability 0.2

		const auto commonDataSize = sizeof(downloadChannelId) + sizeof(sequenceNumber) + sizeof(response);
		auto* const pCommonDataBegin = new uint8_t[commonDataSize];	// TODO: Make smart pointer
		auto* pCommonData = pCommonDataBegin;
		utils::WriteToByteArray(pCommonData, downloadChannelId);
		utils::WriteToByteArray(pCommonData, sequenceNumber);
		utils::WriteToByteArray(pCommonData, response);

		auto opinionData = test::CreateValidOpinionData<uint64_t>(replicatorKeyPairs, pCommonDataBegin, commonDataSize);
		auto pTransaction = test::CreateDownloadApprovalTransaction<model::DownloadApprovalTransaction>(downloadChannelId, sequenceNumber, response, opinionData);

		// Act:
		const auto result = GetValidationResult(*pTransaction, cache.createDelta().toReadOnly());
		delete pCommonDataBegin;

		// Assert:
		EXPECT_EQ(result, ValidationResult::Success);
	}

}}