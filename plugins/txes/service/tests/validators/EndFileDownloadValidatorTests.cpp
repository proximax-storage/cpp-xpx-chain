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

	constexpr auto Current_Height = Height(10);

	namespace {
		using Notification = model::EndFileDownloadNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DownloadEntry& downloadEntry,
				const Key& fileRecipient,
				const Hash256& operationToken,
				const std::vector<model::File>& files = {}) {
			// Arrange:
			auto cache = test::DownloadCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& downloadCacheDelta = delta.sub<cache::DownloadCache>();
				downloadCacheDelta.insert(downloadEntry);
				cache.commit(Current_Height);
			}
			Notification notification(test::GenerateRandomByteArray<Key>(), fileRecipient, operationToken, files.data(), files.size());
			auto pValidator = CreateEndFileDownloadValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
				config::BlockchainConfiguration::Uninitialized(), Current_Height);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenNoFiles) {
		// Arrange:
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			Failure_Service_No_Files_To_Download,
			downloadEntry,
			downloadEntry.FileRecipient,
			downloadEntry.OperationToken);
	}

	TEST(TEST_CLASS, FailureWhenNoDownloadEntry) {
		// Arrange:
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Download_Not_In_Progress,
			downloadEntry,
			downloadEntry.FileRecipient,
			test::GenerateRandomByteArray<Hash256>(),
			{ model::File{ test::GenerateRandomByteArray<Hash256>() } });
	}

	TEST(TEST_CLASS, FailureWhenFileRecipientInvalid) {
		// Arrange:
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			Failure_Service_Invalid_File_Recipient,
			downloadEntry,
			test::GenerateRandomByteArray<Key>(),
			downloadEntry.OperationToken,
			{ model::File{ test::GenerateRandomByteArray<Hash256>() } });
	}

	TEST(TEST_CLASS, FailureWhenDownloadExpired) {
		// Arrange:
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();
		downloadEntry.Height = Current_Height - Height(1);

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Download_Not_In_Progress,
			downloadEntry,
			downloadEntry.FileRecipient,
			downloadEntry.OperationToken,
			{ model::File{ test::GenerateRandomByteArray<Hash256>() } });
	}

	TEST(TEST_CLASS, FailureWhenFileHashNotFound) {
		// Arrange:
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();
		downloadEntry.Height = Current_Height + Height(1);
		downloadEntry.Files = std::set<Hash256>{ test::GenerateRandomByteArray<Hash256>() };

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Download_Not_In_Progress,
			downloadEntry,
			downloadEntry.FileRecipient,
			downloadEntry.OperationToken,
			{ model::File{ test::GenerateRandomByteArray<Hash256>() } });
	}

	TEST(TEST_CLASS, FailureWhenFileHashRedundant) {
		// Arrange:
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();
		downloadEntry.Height = Current_Height + Height(1);
		std::set<Hash256> fileHashes{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		downloadEntry.Files = fileHashes;

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Hash_Redundant,
			downloadEntry,
			downloadEntry.FileRecipient,
			downloadEntry.OperationToken,
			{ model::File{ *fileHashes.begin() }, model::File{ *fileHashes.begin() } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();
		downloadEntry.Height = Current_Height + Height(1);
		std::set<Hash256> fileHashes{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		downloadEntry.Files = fileHashes;

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			downloadEntry,
			downloadEntry.FileRecipient,
			downloadEntry.OperationToken,
			{ model::File{ *fileHashes.begin() }, model::File{ *++fileHashes.begin() } });
	}
}}
