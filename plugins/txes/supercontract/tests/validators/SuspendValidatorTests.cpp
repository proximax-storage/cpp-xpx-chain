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

#define TEST_CLASS SuspendValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Suspend, )

	namespace {
		using Notification = model::SuspendNotification<1>;

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
			auto pValidator = CreateSuspendValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenSignerInvalid) {
		// Arrange:
		state::SuperContractEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setOwner(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Operation_Is_Not_Permitted,
			entry,
			test::GenerateRandomByteArray<Key>());
	}

	TEST(TEST_CLASS, FailureWhenStateIsNotActive) {
		// Arrange:
		state::SuperContractEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setState(state::SuperContractState::Suspended);
		entry.setOwner(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Operation_Is_Not_Permitted,
			entry,
			entry.owner());
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		state::SuperContractEntry entry(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			entry,
			entry.owner());
	}
}}
