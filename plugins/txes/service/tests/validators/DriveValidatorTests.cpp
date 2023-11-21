/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DriveValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Drive, )

	namespace {
		using Notification = model::DriveNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::DriveEntry* pEntry,
				const Key& driveKey,
				model::EntityType transactionType) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::DriveCacheFactory::Create();
			if (pEntry) {
				auto delta = cache.createDelta();
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				driveCacheDelta.insert(*pEntry);
				cache.commit(currentHeight);
			}
			Notification notification(driveKey, transactionType);
			auto pValidator = CreateDriveValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenDriveDoesntExist) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		std::unordered_set<model::EntityType> types({
			model::Entity_Type_JoinToDrive,
			model::Entity_Type_DriveFileSystem,
			model::Entity_Type_FilesDeposit,
			model::Entity_Type_EndDrive,
			model::Entity_Type_DriveFilesReward,
			model::Entity_Type_Start_Drive_Verification,
			model::Entity_Type_End_Drive_Verification,
		});

		// Assert:
		for (const auto& type : types) {
			AssertValidationResult(
				Failure_Service_Drive_Does_Not_Exist,
				nullptr,
				key,
				type);
		}
	}

	TEST(TEST_CLASS, FailureWhenDriveFinished) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		state::DriveEntry entry(key);
		entry.setState(state::DriveState::Finished);
		std::unordered_set<model::EntityType> types({
			model::Entity_Type_DriveFileSystem,
			model::Entity_Type_FilesDeposit,
			model::Entity_Type_JoinToDrive,
			model::Entity_Type_EndDrive,
			model::Entity_Type_Start_Drive_Verification,
			model::Entity_Type_End_Drive_Verification,
		});

		// Assert:
		for (const auto& type : types) {
			AssertValidationResult(
				Failure_Service_Drive_Has_Ended,
				&entry,
				key,
				type);
		}
	}

	TEST(TEST_CLASS, SuccessWhenDriveHasntFinished) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		state::DriveEntry entry(key);
		std::unordered_set<model::EntityType> types({
			model::Entity_Type_DriveFileSystem,
			model::Entity_Type_FilesDeposit,
			model::Entity_Type_JoinToDrive,
			model::Entity_Type_EndDrive,
			model::Entity_Type_Start_Drive_Verification,
			model::Entity_Type_End_Drive_Verification,
		});

		// Assert:
		for (auto state = static_cast<int>(state::DriveState::NotStarted); state < static_cast<int>(state::DriveState::Finished); ++state) {
			entry.setState(static_cast<state::DriveState>(state));
			for (const auto &type : types) {
				AssertValidationResult(
					ValidationResult::Success,
					&entry,
					key,
					type);
			}
		}
	}

	TEST(TEST_CLASS, SuccessWhenOperationNotBlocked) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		state::DriveEntry entry(key);
		std::unordered_set<model::EntityType> types({
			model::Entity_Type_DriveFilesReward,
		});

		// Assert:
		for (auto state = static_cast<int>(state::DriveState::NotStarted); state <= static_cast<int>(state::DriveState::Finished); ++state) {
			entry.setState(static_cast<state::DriveState>(state));
			for (const auto &type : types) {
				AssertValidationResult(
					ValidationResult::Success,
					&entry,
					key,
					type);
			}
		}
	}
}}
