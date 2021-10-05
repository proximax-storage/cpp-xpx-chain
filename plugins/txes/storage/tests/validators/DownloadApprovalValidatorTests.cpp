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

#define TEST_CLASS DownloadApprovalValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(DownloadApproval, )

	namespace {
		using Notification = model::DownloadApprovalNotification<1>;

		constexpr auto Current_Height = Height(10);

		void AssertValidationResult(
				const ValidationResult& expectedResult,
				const cache::CatapultCache& cache,
				const Hash256& downloadChannelId,
				const uint16_t sequenceNumber) {
			// Arrange:
			Notification notification(
					downloadChannelId,
					sequenceNumber
			);
			auto pValidator = CreateDownloadApprovalValidator();

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
		auto& downloadChannelDelta = delta.sub<cache::DownloadChannelCache>();
		auto downloadChannelEntry = test::CreateDownloadChannelEntry();
		downloadChannelDelta.insert(downloadChannelEntry);
		cache.commit(Current_Height);

		// Assert:
		AssertValidationResult(
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
		AssertValidationResult(
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
		AssertValidationResult(
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
		AssertValidationResult(
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
		AssertValidationResult(
				Failure_Storage_Invalid_Sequence_Number,
				cache,
				downloadChannelEntry.id(),
				test::RandomInRange<uint16_t>(downloadChannelEntry.downloadApprovalCount() + 2, upperLimit));
	}
}}
