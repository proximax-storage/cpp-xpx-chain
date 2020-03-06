/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/EndExecuteTransaction.h"
#include "src/model/StartExecuteTransaction.h"
#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS EmbeddedTransactionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EmbeddedTransaction, )

	namespace {
		using Notification = model::AggregateEmbeddedTransactionNotification<1>;

		void AssertValidationResult(ValidationResult expectedResult, const model::EmbeddedTransaction& transaction, size_t index) {
			// Arrange:
			Notification notification(Key(), transaction, index, 0, nullptr);
			auto pValidator = CreateEmbeddedTransactionValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenEndExecuteMisplaced) {
		// Assert:
		AssertValidationResult(
			Failure_SuperContract_End_Execute_Transaction_Misplaced,
			*test::CreateEndExecuteTransaction<model::EmbeddedEndExecuteTransaction>(5),
			1);
	}

	TEST(TEST_CLASS, SuccessWhenValidEndExecute) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			*test::CreateEndExecuteTransaction<model::EmbeddedEndExecuteTransaction>(5),
			0);
	}

	TEST(TEST_CLASS, SuccessWhenNotEndExecute) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			*test::CreateStartExecuteTransaction<model::EmbeddedStartExecuteTransaction>(5, 5, 5),
			0);
	}
}}
