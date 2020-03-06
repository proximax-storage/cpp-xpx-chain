/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS EndOperationValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EndOperation, )

	namespace {
		using Notification = model::EndOperationNotification<1>;

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
			Notification notification(signer, Hash256(), nullptr, 0, model::Operation_Result_None);
			auto pValidator = CreateEndOperationValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenEndOperationEndsSuperContractExecution) {
		// Arrange:
		state::SuperContractEntry entry(test::GenerateRandomByteArray<Key>());

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Operation_Is_Not_Permitted,
			entry,
			entry.key());
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			state::SuperContractEntry(test::GenerateRandomByteArray<Key>()),
			test::GenerateRandomByteArray<Key>());
	}
}}
