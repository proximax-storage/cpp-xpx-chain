/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/cache/DownloadCache.h"
#include "src/cache/DriveCache.h"
#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS StartFileDownloadValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(StartFileDownload, )

	namespace {
		using Notification = model::StartFileDownloadNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry& driveEntry,
				const state::DownloadEntry& downloadEntry,
				const std::vector<model::DownloadAction>& files = {}) {
			// Arrange:
			Height currentHeight(10);
			auto cache = test::DownloadCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(driveEntry);
				auto& downloadCacheDelta = delta.sub<cache::DownloadCache>();
				downloadCacheDelta.insert(downloadEntry);
				cache.commit(currentHeight);
			}
			Notification notification(downloadEntry.DriveKey, downloadEntry.FileRecipient, downloadEntry.OperationToken, files.data(), files.size());
			auto pValidator = CreateStartFileDownloadValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
				config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}
	TEST(TEST_CLASS, FailureWhenFileRecipientIsReplicator) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		driveEntry.replicators().emplace(replicatorKey, state::ReplicatorInfo{ Height(1), Height(10), {}, {} });
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.DriveKey = driveKey;
		downloadEntry.FileRecipient = replicatorKey;

		// Assert:
		AssertValidationResult(
			Failure_Service_Operation_Is_Not_Permitted,
			driveEntry,
			downloadEntry);
	}

	TEST(TEST_CLASS, FailureWhenNoFilesToDownload) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.DriveKey = driveKey;
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			Failure_Service_No_Files_To_Download,
			driveEntry,
			downloadEntry);
	}

	TEST(TEST_CLASS, FailureWhenFileDoesntExist) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.DriveKey = driveKey;
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Doesnt_Exist,
			driveEntry,
			downloadEntry,
			{ { { test::GenerateRandomByteArray<Hash256>() }, test::Random() } });
	}

	TEST(TEST_CLASS, FailureWhenFileSizeInvalid) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		auto fileSize = test::Random();
		driveEntry.files().emplace(fileHash, state::FileInfo{ fileSize });
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.DriveKey = driveKey;
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Size_Invalid,
			driveEntry,
			downloadEntry,
			{ { { fileHash }, fileSize - 1 } });
	}

	TEST(TEST_CLASS, FailureWhenFileHashRedundant) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		auto fileSize = test::Random();
		driveEntry.files().emplace(fileHash, state::FileInfo{ fileSize });
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.DriveKey = driveKey;
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Hash_Redundant,
			driveEntry,
			downloadEntry,
			{ { { fileHash }, fileSize }, { { fileHash }, fileSize } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		auto fileSize = test::Random();
		driveEntry.files().emplace(fileHash, state::FileInfo{ fileSize });
		state::DownloadEntry downloadEntry(test::GenerateRandomByteArray<Hash256>());
		downloadEntry.DriveKey = driveKey;
		downloadEntry.FileRecipient = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			downloadEntry,
			{ { { fileHash }, fileSize } });
	}
}}
