/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/cache/DownloadCache.h"
#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS EndFileDownloadValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EndFileDownload, )

	namespace {
		using Notification = model::EndFileDownloadNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DownloadEntry& downloadEntry,
				const Key& driveKey,
				const Key& fileRecipient,
				const Hash256& operationToken,
				const std::vector<model::File>& files = {}) {
			// Arrange:
			Height currentHeight(10);
			auto cache = test::DownloadCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& downloadCacheDelta = delta.sub<cache::DownloadCache>();
				downloadCacheDelta.insert(downloadEntry);
				cache.commit(currentHeight);
			}
			Notification notification(driveKey, fileRecipient, operationToken, files.data(), files.size());
			auto pValidator = CreateEndFileDownloadValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
				config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenNoDownloadEntry) {
		// Arrange:
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Download_Not_In_Progress,
			downloadEntry,
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Hash256>());
	}

	TEST(TEST_CLASS, FailureWhenFileRecipientNotFound) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DownloadEntry downloadEntry(driveKey);

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Download_Not_In_Progress,
			downloadEntry,
			driveKey,
			test::GenerateRandomByteArray<Key>(),
			test::GenerateRandomByteArray<Hash256>());
	}

	TEST(TEST_CLASS, FailureWhenOperationTokenNotFound) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		auto fileRecipient = test::GenerateRandomByteArray<Key>();
		state::DownloadEntry downloadEntry(driveKey);
		std::set<Hash256> fileHashes;
		downloadEntry.fileRecipients()[fileRecipient].emplace(test::GenerateRandomByteArray<Hash256>(), fileHashes);

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Download_Not_In_Progress,
			downloadEntry,
			driveKey,
			fileRecipient,
			test::GenerateRandomByteArray<Hash256>());
	}

	TEST(TEST_CLASS, FailureWhenFileHashNotFound) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		auto fileRecipient = test::GenerateRandomByteArray<Key>();
		auto operationToken = test::GenerateRandomByteArray<Hash256>();
		state::DownloadEntry downloadEntry(driveKey);
		std::set<Hash256> fileHashes{ test::GenerateRandomByteArray<Hash256>() };
		downloadEntry.fileRecipients()[fileRecipient].emplace(operationToken, fileHashes);

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Download_Not_In_Progress,
			downloadEntry,
			driveKey,
			fileRecipient,
			operationToken,
			{ model::File{ test::GenerateRandomByteArray<Hash256>() } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		auto fileRecipient = test::GenerateRandomByteArray<Key>();
		auto operationToken = test::GenerateRandomByteArray<Hash256>();
		state::DownloadEntry downloadEntry(driveKey);
		std::set<Hash256> fileHashes{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		downloadEntry.fileRecipients()[fileRecipient].emplace(operationToken, fileHashes);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			downloadEntry,
			driveKey,
			fileRecipient,
			operationToken,
			{ model::File{ *fileHashes.begin() }, model::File{ *++fileHashes.begin() } });
	}
}}
