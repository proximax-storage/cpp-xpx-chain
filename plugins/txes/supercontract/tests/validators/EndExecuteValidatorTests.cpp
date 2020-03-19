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

#define TEST_CLASS EndExecuteValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EndExecute, )

	namespace {
		using Notification = model::EndExecuteNotification<1>;

		void AssertValidationResult(
				ValidationResult expectedResult,
				const state::SuperContractEntry& entry) {
			// Arrange:
			Height currentHeight(1);
			auto cache = test::SuperContractCacheFactory::Create();
			{
				auto delta = cache.createDelta();
				auto& superContractCacheDelta = delta.sub<cache::SuperContractCache>();
				superContractCacheDelta.insert(entry);
				cache.commit(currentHeight);
			}
			Notification notification(entry.key());
			auto pValidator = CreateEndExecuteValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache,
					config::BlockchainConfiguration::Uninitialized(), currentHeight);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenExecutionNotInProgress) {
		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Execution_Is_Not_In_Progress,
			state::SuperContractEntry(test::GenerateRandomByteArray<Key>()));
	}

	TEST(TEST_CLASS, Success) {
		// Arrange:
		state::SuperContractEntry entry(test::GenerateRandomByteArray<Key>());
		entry.setExecutionCount(1);

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			entry);
	}
}}
