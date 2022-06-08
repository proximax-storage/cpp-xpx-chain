/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/OperationIdentifyTransaction.h"
#include "src/model/StartOperationTransaction.h"
#include "src/model/EndOperationTransaction.h"
#include "src/validators/Validators.h"
#include "tests/test/OperationTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"

namespace catapult { namespace validators {

#define TEST_CLASS AggregateTransactionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(AggregateTransaction, )

	namespace {
		using Notification = model::AggregateCosignaturesNotification<1>;

		constexpr auto Num_Transactions = 3u;

		auto CreateOperationIdentifyTransaction() {
			return test::CreateOperationIdentifyTransaction<model::EmbeddedOperationIdentifyTransaction>();
		}

		auto CreateStartOperationTransaction() {
			return test::CreateStartOperationTransaction<model::EmbeddedStartOperationTransaction>(5, 5);
		}

		auto CreateEndOperationTransaction() {
			return test::CreateEndOperationTransaction<model::EmbeddedEndOperationTransaction>(5);
		}

		void AssertValidationResult(ValidationResult expectedResult, const model::EmbeddedTransaction* pTransactions) {
			// Arrange:
			Notification notification(Key(), Num_Transactions, pTransactions, 0, static_cast<model::Cosignature<SignatureLayout::Raw> *>(nullptr));
			auto pValidator = CreateAggregateTransactionValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenEndOperationMisplaced) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateStartOperationTransaction());
		buffer.addTransaction(CreateEndOperationTransaction());
		buffer.addTransaction(CreateStartOperationTransaction());

		// Assert:
		AssertValidationResult(
			Failure_Operation_End_Transaction_Misplaced,
			buffer.transactions());
	}

	TEST(TEST_CLASS, FailureWhenOperationIdentifyMisplaced) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateStartOperationTransaction());
		buffer.addTransaction(CreateOperationIdentifyTransaction());
		buffer.addTransaction(CreateStartOperationTransaction());

		// Assert:
		AssertValidationResult(
			Failure_Operation_Identify_Transaction_Misplaced,
			buffer.transactions());
	}

	TEST(TEST_CLASS, FailureWhenOperationIdentifyAggregatedWithEndOperation) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateOperationIdentifyTransaction());
		buffer.addTransaction(CreateStartOperationTransaction());
		buffer.addTransaction(CreateEndOperationTransaction());

		// Assert:
		AssertValidationResult(
			Failure_Operation_Identify_Transaction_Aggregated_With_End_Operation,
			buffer.transactions());
	}

	TEST(TEST_CLASS, SuccessWhenValidOperationIdentify) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateOperationIdentifyTransaction());
		buffer.addTransaction(CreateStartOperationTransaction());
		buffer.addTransaction(CreateStartOperationTransaction());

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			buffer.transactions());
	}

	TEST(TEST_CLASS, SuccessWhenValidEndOperation) {
		// Arrange:
		test::TransactionBuffer buffer;
		buffer.addTransaction(CreateStartOperationTransaction());
		buffer.addTransaction(CreateStartOperationTransaction());
		buffer.addTransaction(CreateEndOperationTransaction());

		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			buffer.transactions());
	}
}}
