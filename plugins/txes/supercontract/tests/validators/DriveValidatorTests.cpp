/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/service/src/cache/DriveCache.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"
#include "src/model/SuperContractEntityType.h"
#include "src/validators/Validators.h"
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

	TEST(TEST_CLASS, FailureWhenDriveEnded) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		state::DriveEntry entry(key);
		entry.setState(state::DriveState::Finished);
		std::unordered_set<model::EntityType> types({
		    model::Entity_Type_Deploy,
		    model::Entity_Type_StartExecute,
		    model::Entity_Type_EndExecute,
		});

		// Assert:
		for (const auto& type : types) {
			AssertValidationResult(
				Failure_SuperContract_Drive_Has_Ended,
				&entry,
				key,
				type);
		}
	}

	TEST(TEST_CLASS, SuccessWhenDriveHasntEnded) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		state::DriveEntry entry(key);
		std::unordered_set<model::EntityType> types({
		    model::Entity_Type_Deploy,
		    model::Entity_Type_StartExecute,
		    model::Entity_Type_EndExecute,
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
}}
