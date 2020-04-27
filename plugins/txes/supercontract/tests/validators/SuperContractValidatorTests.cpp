/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SuperContractCache.h"
#include "src/validators/Validators.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS SuperContractValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(SuperContract, )

	namespace {
		using Notification = model::SuperContractNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::SuperContractEntry& superContractEntry,
				const Key& key,
				model::EntityType transactionType) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::SuperContractCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& superContractCacheDelta = delta.sub<cache::SuperContractCache>();
				superContractCacheDelta.insert(superContractEntry);
				cache.commit(currentHeight);
			}
			Notification notification(key, transactionType);
			auto pValidator = CreateSuperContractValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenSuperContractNotFound) {
		// Arrange:
		state::SuperContractEntry superContractEntry(test::GenerateRandomByteArray<Key>());
		std::unordered_set<model::EntityType> types({
			model::Entity_Type_Deploy,
			model::Entity_Type_StartExecute,
			model::Entity_Type_EndExecute,
			model::Entity_Type_UploadFile,
			model::Entity_Type_Deactivate,
			model::Entity_Type_Suspend,
			model::Entity_Type_Resume,
		});

		// Assert:
		for (const auto& type : types) {
			AssertValidationResult(
				Failure_SuperContract_SuperContract_Does_Not_Exist,
				superContractEntry,
				test::GenerateRandomByteArray<Key>(),
				type);
		}
	}

	TEST(TEST_CLASS, FailureWhenSuperContractDeactivated) {
		for (const auto& state : {state::SuperContractState::DeactivatedByParticipant, state::SuperContractState::DeactivatedByDriveEnd}) {
			// Arrange:
			state::SuperContractEntry superContractEntry(test::GenerateRandomByteArray<Key>());
			superContractEntry.setState(state);
			std::unordered_set<model::EntityType> types({
				model::Entity_Type_StartExecute,
				model::Entity_Type_Deactivate,
				model::Entity_Type_Suspend,
				model::Entity_Type_Resume,
			});

			// Assert:
			for (const auto& type : types) {
				AssertValidationResult(
					Failure_SuperContract_Operation_Is_Not_Permitted,
					superContractEntry,
					superContractEntry.key(),
					type);
			}
		}
	}

	TEST(TEST_CLASS, SuccessWhenSuperContractActive) {
		// Arrange:
		state::SuperContractEntry superContractEntry(test::GenerateRandomByteArray<Key>());
		superContractEntry.setState(state::SuperContractState::Active);
		std::unordered_set<model::EntityType> types({
			model::Entity_Type_Deploy,
			model::Entity_Type_StartExecute,
			model::Entity_Type_EndExecute,
			model::Entity_Type_UploadFile,
			model::Entity_Type_Deactivate,
			model::Entity_Type_Suspend,
			model::Entity_Type_Resume,
		});

		// Assert:
		for (const auto& type : types) {
			AssertValidationResult(
				ValidationResult::Success,
				superContractEntry,
				superContractEntry.key(),
				type);
		}
	}

	TEST(TEST_CLASS, SuccessWhenSuperContractDeactivatedAndTransactionEnabled) {
		for (const auto& state : {state::SuperContractState::DeactivatedByParticipant, state::SuperContractState::DeactivatedByDriveEnd}) {
			// Arrange:
			state::SuperContractEntry superContractEntry(test::GenerateRandomByteArray<Key>());
			superContractEntry.setState(state);
			std::unordered_set<model::EntityType> types({
				model::Entity_Type_Deploy,
				model::Entity_Type_EndExecute,
				model::Entity_Type_UploadFile,
			});

			// Assert:
			for (const auto& type : types) {
				AssertValidationResult(
					ValidationResult::Success,
					superContractEntry,
					superContractEntry.key(),
					type);
			}
		}
	}
}}
