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
#include "catapult/model/VerifiableEntity.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DeadlineValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Deadline, model::BlockChainConfiguration::Uninitialized())

	namespace {
		const auto Block_Time = Timestamp(8888);
		const auto Max_Transaction_Lifetime = []() { return utils::TimeSpan::FromHours(2); }();

		void AssertValidationResult(ValidationResult expectedResult, Timestamp deadline) {
			// Arrange:
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.MaxTransactionLifetime = Max_Transaction_Lifetime;
			auto cache = test::CreateEmptyCatapultCache(config);
			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto resolverContext = test::CreateResolverContextXor();
			auto context = ValidatorContext(Height(123), Block_Time, model::NetworkInfo(), resolverContext, readOnlyCache);
			auto pValidator = CreateDeadlineValidator(config);

			model::TransactionNotification<1> notification(Key(), Hash256(), model::EntityType(), deadline);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	// region basic tests

	TEST(TEST_CLASS, SuccessWhenTransactionDeadlineIsValid) {
		// Arrange:
		auto deadline = Block_Time + utils::TimeSpan::FromHours(1);

		// Assert:
		AssertValidationResult(ValidationResult::Success, deadline);
	}

	TEST(TEST_CLASS, SuccessWhenTransactionDeadlineIsEqualToTimestampPlusLifetime) {
		// Arrange:
		auto deadline = Block_Time + utils::TimeSpan::FromHours(2);

		// Assert:
		AssertValidationResult(ValidationResult::Success, deadline);
	}

	TEST(TEST_CLASS, SuccessWhenTransactionDeadlineIsEqualToBlockTime) {
		// Arrange:
		auto deadline = Block_Time;

		// Assert:
		AssertValidationResult(ValidationResult::Success, deadline);
	}

	TEST(TEST_CLASS, FailureWhenTransactionDeadlineIsLessThanBlockTime) {
		// Arrange:
		auto deadline = Timestamp(Block_Time.unwrap() - 1);

		// Assert:
		AssertValidationResult(Failure_Core_Past_Deadline, deadline);
	}

	TEST(TEST_CLASS, FailureWhenTransactionDeadlineIsLargerThanBlockTimePlusLifetime) {
		// Arrange:
		auto deadline = Block_Time + utils::TimeSpan::FromHours(3);

		// Assert:
		AssertValidationResult(Failure_Core_Future_Deadline, deadline);
	}

	// endregion
}}
