/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/operation/src/model/OperationIdentifyTransaction.h"
#include "src/model/EndExecuteTransaction.h"
#include "src/model/StartExecuteTransaction.h"
#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/SuperContractTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS AggregateTransactionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(AggregateTransaction, )

	namespace {
		using Notification = model::AggregateCosignaturesNotification<1>;

		constexpr auto Num_Transactions = 3u;

		auto CreateOperationIdentifyTransaction() {
			return test::CreateOperationIdentifyTransaction<model::EmbeddedOperationIdentifyTransaction>();
		}

		auto CreateStartExecuteTransaction() {
			return test::CreateStartExecuteTransaction<model::EmbeddedStartExecuteTransaction>(5, 5, 5);
		}

		auto CreateEndExecuteTransaction() {
			return test::CreateEndExecuteTransaction<model::EmbeddedEndExecuteTransaction>(5);
		}

		void AssertValidationResult(ValidationResult expectedResult, const model::EmbeddedTransaction* pTransactions) {
			// Arrange:
			Notification notification(Key(), Num_Transactions, pTransactions, 0, nullptr);
			auto pValidator = CreateAggregateTransactionValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenEndExecuteMisplaced) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateStartExecuteTransaction());
		buffer.addTransaction(CreateEndExecuteTransaction());
		buffer.addTransaction(CreateStartExecuteTransaction());

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_End_Execute_Transaction_Misplaced,
			buffer.transactions());
	}

	TEST(TEST_CLASS, FailureWhenOperationIdentifyMisplaced) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateStartExecuteTransaction());
		buffer.addTransaction(CreateOperationIdentifyTransaction());
		buffer.addTransaction(CreateStartExecuteTransaction());

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Operation_Identify_Transaction_Misplaced,
			buffer.transactions());
	}

	TEST(TEST_CLASS, FailureWhenOperationIdentifyAggregatedWithEndExecute) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateOperationIdentifyTransaction());
		buffer.addTransaction(CreateStartExecuteTransaction());
		buffer.addTransaction(CreateEndExecuteTransaction());

		// Assert:
		AssertValidationResult(
			Failure_SuperContract_Operation_Identify_Transaction_Aggregated_With_End_Execute,
			buffer.transactions());
	}

	TEST(TEST_CLASS, SuccessWhenValidOperationIdentify) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateOperationIdentifyTransaction());
		buffer.addTransaction(CreateStartExecuteTransaction());
		buffer.addTransaction(CreateStartExecuteTransaction());

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			buffer.transactions());
	}

	TEST(TEST_CLASS, SuccessWhenValidEndExecute) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateStartExecuteTransaction());
		buffer.addTransaction(CreateStartExecuteTransaction());
		buffer.addTransaction(CreateEndExecuteTransaction());

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			buffer.transactions());
	}
}}
