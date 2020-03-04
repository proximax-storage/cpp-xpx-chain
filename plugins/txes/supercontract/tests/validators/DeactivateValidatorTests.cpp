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

#define TEST_CLASS DeactivateValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Deactivate, )

	namespace {
		using Notification = model::DeactivateNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::SuperContractEntry& entry,
				const Key& signer) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::SuperContractCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& superContractCacheDelta = delta.sub<cache::SuperContractCache>();
				superContractCacheDelta.insert(entry);
				cache.commit(currentHeight);
			}
			Notification notification(signer, entry.key());
			auto pValidator = CreateDeactivateValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		auto CreateSuperContractEntry() {
			state::SuperContractEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setOwner(test::GenerateRandomByteArray<Key>());
			entry.setMainDriveKey(test::GenerateRandomByteArray<Key>());

			return entry;
		}
	}

	TEST(TEST_CLASS, FailureWhenSignerInvalid) {
		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Operation_Is_Not_Permitted,
			CreateSuperContractEntry(),
			test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, FailureWhenExecutionInProgress_SignerIsOwner) {
		// Arrange:
		auto entry = CreateSuperContractEntry();
		entry.setExecutionCount(10);

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Execution_Is_In_Progress,
			entry,
			entry.owner());
	}

	TEST(TEST_CLASS, FailureWhenExecutionInProgress_SignerIsMainDrive) {
		// Arrange:
		auto entry = CreateSuperContractEntry();
		entry.setExecutionCount(10);

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Execution_Is_In_Progress,
			entry,
			entry.mainDriveKey());
	}

	TEST(TEST_CLASS, Success_SignerIsOwner) {
		// Arrange:
		auto entry = CreateSuperContractEntry();

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			entry,
			entry.owner());
	}

	TEST(TEST_CLASS, Success_SignerIsMainDrive) {
		// Arrange:
		auto entry = CreateSuperContractEntry();

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			entry,
			entry.mainDriveKey());
	}
}}
