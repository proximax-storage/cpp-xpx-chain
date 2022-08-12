/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS DriveFileSystemValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(DriveFileSystem, )

	namespace {
		using Notification = model::DriveFileSystemNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry& driveEntry,
				const state::SuperContractEntry& superContractEntry,
				const std::vector<model::RemoveAction>& removeActions) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::SuperContractCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(driveEntry);
				auto& superContractCacheDelta = delta.sub<cache::SuperContractCache>();
				superContractCacheDelta.insert(superContractEntry);
				cache.commit(currentHeight);
			}
			Notification notification(driveEntry.key(), Key(), Hash256(), Hash256(), 0, nullptr, removeActions.size(), removeActions.data());
			auto pValidator = CreateDriveFileSystemValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenRemovingSuperContractFile) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		auto file = test::GenerateRandomByteArray<Hash256>();
		driveEntry.files().emplace(file, state::FileInfo{ 1000 });
		auto superContract = test::GenerateRandomByteArray<Key>();
		driveEntry.coowners().insert(superContract);
		state::SuperContractEntry superContractEntry(superContract);
		superContractEntry.setFileHash(file);
		superContractEntry.setState(state::SuperContractState::Active);

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Remove_Super_Contract_File,
			driveEntry,
			superContractEntry,
			{ { { { file }, 1000 } } });
	}

	TEST(TEST_CLASS, SuccessWhenNoRemoveActions) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		auto file = test::GenerateRandomByteArray<Hash256>();
		driveEntry.files().emplace(file, state::FileInfo{ 1000 });
		auto superContract = test::GenerateRandomByteArray<Key>();
		driveEntry.coowners().insert(superContract);
		state::SuperContractEntry superContractEntry(superContract);
		superContractEntry.setFileHash(file);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			superContractEntry,
			{});
	}

	TEST(TEST_CLASS, SuccessWhenSuperContractIsNotDriveOwner) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		auto file = test::GenerateRandomByteArray<Hash256>();
		driveEntry.files().emplace(file, state::FileInfo{ 1000 });
		auto superContract = test::GenerateRandomByteArray<Key>();
		state::SuperContractEntry superContractEntry(superContract);
		superContractEntry.setFileHash(file);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			superContractEntry,
			{ { { { file }, 1000 } } });
	}

	TEST(TEST_CLASS, SuccessWhenSuperContractDeactivated) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		auto file = test::GenerateRandomByteArray<Hash256>();
		driveEntry.files().emplace(file, state::FileInfo{ 1000 });
		auto superContract = test::GenerateRandomByteArray<Key>();
		driveEntry.coowners().insert(superContract);
		state::SuperContractEntry superContractEntry(superContract);
		superContractEntry.setFileHash(file);

		// Assert:
		for (const auto& state : {state::SuperContractState::DeactivatedByParticipant, state::SuperContractState::DeactivatedByDriveEnd}) {
			superContractEntry.setState(state);
			AssertValidationResult(
				ValidationResult::Success,
				driveEntry,
				superContractEntry,
				{ { { { file }, 1000 } } });
		}
	}

	TEST(TEST_CLASS, SuccessWhenNotRemovingSuperContractFile) {
		// Arrange:
		state::DriveEntry driveEntry(test::GenerateRandomByteArray<Key>());
		auto file = test::GenerateRandomByteArray<Hash256>();
		driveEntry.files().emplace(file, state::FileInfo{ 1000 });
		auto superContract = test::GenerateRandomByteArray<Key>();
		driveEntry.coowners().insert(superContract);
		state::SuperContractEntry superContractEntry(superContract);
		superContractEntry.setFileHash(file);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			driveEntry,
			superContractEntry,
			{ { { { test::GenerateRandomByteArray<Hash256>() }, 1000 } } });
	}
}}
