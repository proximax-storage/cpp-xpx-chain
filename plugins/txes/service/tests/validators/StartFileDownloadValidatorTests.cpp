/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
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
				const Key& recipient,
				const std::vector<model::DownloadAction>& files = {}) {
			// Arrange:
			Height currentHeight(10);
			auto cache = test::DriveCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(driveEntry);
				cache.commit(currentHeight);
			}
			Notification notification(driveEntry.key(), recipient, files.data(), files.size());
			auto pValidator = CreateStartFileDownloadValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
				config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenDriveIsNotInProgress) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);

		// Assert:
		AssertValidationResult(
			Failure_Service_Drive_Is_Not_In_Progress,
			driveEntry,
			test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, FailureWhenRecipientIsReplicator) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		driveEntry.replicators().emplace(replicatorKey, state::ReplicatorInfo{ Height(1), Height(10), {}, {} });

		// Assert:
		AssertValidationResult(
			Failure_Service_Operation_Is_Not_Permitted,
			driveEntry,
			replicatorKey);
	}

	TEST(TEST_CLASS, FailureWhenNoFilesToDownload) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);

		// Assert:
		AssertValidationResult(
			Failure_Service_No_Files_To_Download,
			driveEntry,
			test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, FailureWhenFileDoesntExist) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Doesnt_Exist,
			driveEntry,
			test::GenerateRandomByteArray<Key>(),
			{ model::DownloadAction{ { test::GenerateRandomByteArray<Hash256>() }, test::Random() } });
	}

	TEST(TEST_CLASS, FailureWhenFileSizeInvalid) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		auto fileSize = test::Random();
		driveEntry.files().emplace(fileHash, state::FileInfo{ fileSize });

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Size_Invalid,
			driveEntry,
			test::GenerateRandomByteArray<Key>(),
			{ model::DownloadAction{ { fileHash }, fileSize - 1 } });
	}

	TEST(TEST_CLASS, FailureWhenFileHashRedundant) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		auto fileSize = test::Random();
		driveEntry.files().emplace(fileHash, state::FileInfo{ fileSize });

		// Assert:
		AssertValidationResult(
			Failure_Service_File_Hash_Redundant,
			driveEntry,
			test::GenerateRandomByteArray<Key>(),
			{ model::DownloadAction{ { fileHash }, fileSize }, model::DownloadAction{ { fileHash }, fileSize } });
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		state::DriveEntry driveEntry(driveKey);
		driveEntry.setState(state::DriveState::InProgress);
		auto fileHash = test::GenerateRandomByteArray<Hash256>();
		auto fileSize = test::Random();
		driveEntry.files().emplace(fileHash, state::FileInfo{ fileSize });

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			test::GenerateRandomByteArray<Key>(),
			{ model::DownloadAction{ { fileHash }, fileSize } });
	}
}}
