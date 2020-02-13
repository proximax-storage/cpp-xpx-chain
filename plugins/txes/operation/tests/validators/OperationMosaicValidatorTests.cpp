/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/validators/Validators.h"
#include "tests/test/OperationTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS OperationMosaicValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(OperationMosaic, )

	namespace {
		using Notification = model::OperationMosaicNotification<1>;

		void AssertValidationResult(ValidationResult expectedResult, const std::vector<model::UnresolvedMosaic> mosaics) {
			// Arrange:
			Notification notification(mosaics.data(), mosaics.size());
			auto pValidator = CreateOperationMosaicValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenMosaicRedundant) {
		// Assert:
		AssertValidationResult(
			Failure_Operation_Mosaic_Redundant,
			{
				{ UnresolvedMosaicId(1), Amount(10) },
				{ UnresolvedMosaicId(1), Amount(20) },
			});
	}

	TEST(TEST_CLASS, FailureWhenZeroMosaicAmount) {
		// Assert:
		AssertValidationResult(
			Failure_Operation_Zero_Mosaic_Amount,
			{
				{ UnresolvedMosaicId(1), Amount(0) },
			});
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			{
				{ UnresolvedMosaicId(1), Amount(10) },
				{ UnresolvedMosaicId(2), Amount(20) },
			});
	}
}}
