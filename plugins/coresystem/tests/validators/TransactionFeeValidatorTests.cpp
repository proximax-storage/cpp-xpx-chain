/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/validators/Validators.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS TransactionFeeValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(TransactionFee,)

	namespace {
		void AssertValidationResult(ValidationResult expectedResult, const model::Transaction& transaction, Amount fee, Amount maxFee) {
			// Arrange:
			model::TransactionFeeNotification<1> notification(transaction, UnresolvedMosaicId(0), fee, maxFee);
			auto pValidator = CreateTransactionFeeValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "size = " << transaction.Size << ", fee = " << fee << ", max fee = " << maxFee;
		}

		void AssertValidationResult(ValidationResult expectedResult, model::Transaction& transaction, Amount maxFee) {
			AssertValidationResult(expectedResult, transaction, maxFee, maxFee);
		}
	}

	// region fee <= max fee

	TEST(TEST_CLASS, SuccessWhenFeeIsLessThanMaxFee) {
		// Assert:
		{
			model::Transaction tx;
			tx.Size = 200;
			AssertValidationResult(ValidationResult::Success, tx, Amount(0), Amount(234));
		}
		{
			model::Transaction tx;
			tx.Size = 300;
			AssertValidationResult(ValidationResult::Success, tx, Amount(123), Amount(234));
		}
		{
			model::Transaction tx;
			tx.Size = 400;
			AssertValidationResult(ValidationResult::Success, tx, Amount(233), Amount(234));
		}
	}

	TEST(TEST_CLASS, SuccessWhenFeeIsEqualToMaxFee) {
		// Assert:
		{
			model::Transaction tx;
			tx.Size = 200;
			AssertValidationResult(ValidationResult::Success, tx, Amount(234), Amount(234));
		}
	}

	TEST(TEST_CLASS, FailureWhenFeeIsGreaterThanMaxFee) {
		// Assert:
		{
			model::Transaction tx;
			tx.Size = 300;
			AssertValidationResult(Failure_Core_Invalid_Transaction_Fee, tx, Amount(235), Amount(234));
		}
		{
			model::Transaction tx;
			tx.Size = 400;
			AssertValidationResult(Failure_Core_Invalid_Transaction_Fee, tx, Amount(1000), Amount(234));
		}
	}

	// endregion

	// region max fee multiplier can't overflow

	TEST(TEST_CLASS, SuccessWhenMaxFeeMultiplierIsLessThanMax) {
		// Assert:
		{
			model::Transaction tx;
			tx.Size = 200;
			AssertValidationResult(ValidationResult::Success, tx, Amount(200ull * 0xFFFF'FFFE));
		}
		{
			model::Transaction tx;
			tx.Size = 300;
			AssertValidationResult(ValidationResult::Success, tx, Amount(300ull * 0xFFFF'FF00));
		}
		{
			model::Transaction tx;
			tx.Size = 400;
			AssertValidationResult(ValidationResult::Success, tx, Amount(400ull * 0xFFFF'FFFE));
		}
	}

	TEST(TEST_CLASS, SuccessWhenMaxFeeMultiplierIsEqualToMax) {
		// Assert:
		{
			model::Transaction tx;
			tx.Size = 200;
			AssertValidationResult(ValidationResult::Success, tx, Amount(200ull * 0xFFFF'FFFF));
		}
		{
			model::Transaction tx;
			tx.Size = 300;
			AssertValidationResult(ValidationResult::Success, tx, Amount(300ull * 0xFFFF'FFFF));
		}
		{
			model::Transaction tx;
			tx.Size = 400;
			AssertValidationResult(ValidationResult::Success, tx, Amount(400ull * 0xFFFF'FFFF));
		}
	}

	TEST(TEST_CLASS, FailureWhenMaxFeeMultiplierIsGreaterThanMax) {
		// Assert:
		{
			model::Transaction tx;
			tx.Size = 200;
			AssertValidationResult(Failure_Core_Invalid_Transaction_Fee, tx, Amount(200ull * 0xFFFF'FFFF + 1));
		}
		{
			model::Transaction tx;
			tx.Size = 300;
			AssertValidationResult(Failure_Core_Invalid_Transaction_Fee, tx, Amount(300ull * 0x1'0000'FFFF));
		}
		{
			model::Transaction tx;
			tx.Size = 400;
			AssertValidationResult(Failure_Core_Invalid_Transaction_Fee, tx, Amount(400ull * 0x1'0000'0000));
		}
	}

	// endregion
}}
