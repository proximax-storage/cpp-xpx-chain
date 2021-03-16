/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "catapult/model/Block.h"
#include "catapult/validators/ValidatorContext.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS GreedValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Greed,)

	namespace {
		void AssertValidationResult(
			ValidationResult expectedResult,
			uint32_t feeInterest,
			uint32_t feeInterestDenominator) {
			// Arrange:
			auto notification = test::CreateBlockNotification();
			notification.FeeInterest = feeInterest;
			notification.FeeInterestDenominator = feeInterestDenominator;
			auto pValidator = CreateGreedValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureIfDenominatorIsZero) {
		// Assert:
		AssertValidationResult(Failure_Core_Invalid_FeeInterestDenominator, 0, 0);
		AssertValidationResult(Failure_Core_Invalid_FeeInterestDenominator, 1, 0);
	}

	TEST(TEST_CLASS, FailureIfDenominatorIsLessThanNumerator) {
		// Assert:
		AssertValidationResult(Failure_Core_Invalid_FeeInterest, 2, 1);
	}

	TEST(TEST_CLASS, SuccessIfAllCriteriaAreMet) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, 1, 1);
		AssertValidationResult(ValidationResult::Success, 1, 2);
	}
}}
