/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/OperationTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS StartOperationValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(StartOperation, )

	namespace {
		using Notification = model::StartOperationNotification<1>;

		void AssertValidationResult(ValidationResult expectedResult, const std::vector<Key>& executors) {
			// Arrange:
			Notification notification(Hash256(), Key(), executors.data(), executors.size(), nullptr, 0, BlockDuration());
			auto pValidator = CreateStartOperationValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenExecutorRedundant) {
		// Arrange:
		auto executor = test::GenerateRandomByteArray<Key>();

		// Assert:
		AssertValidationResult(
			Failure_Operation_Executor_Redundant,
			{ executor, executor });
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>(), });
	}
}}
