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
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS DeadlineValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Deadline)

	namespace {
		constexpr auto Block_Time = Timestamp(8888);
		const auto Max_Transaction_Lifetime = []() { return utils::TimeSpan::FromHours(2); }();
		constexpr auto TimeSpanFromHours = utils::TimeSpan::FromHours;

		void AssertValidationResult(ValidationResult expectedResult, Timestamp deadline, const utils::TimeSpan& maxCustomLifetime, bool enableDeadlineValidation = true) {
			// Arrange:
			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Network.MaxTransactionLifetime = Max_Transaction_Lifetime;
			mutableConfig.Network.EnableDeadlineValidation = enableDeadlineValidation;
			auto config = mutableConfig.ToConst();
			auto cache = test::CreateEmptyCatapultCache(config);
			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto resolverContext = test::CreateResolverContextXor();
			auto context = ValidatorContext(config, Height(123), Block_Time, resolverContext, readOnlyCache);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto pValidator = CreateDeadlineValidator();

			model::TransactionDeadlineNotification<1> notification(deadline, maxCustomLifetime);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "deadline " << deadline << ", maxCustomLifetime " << maxCustomLifetime;
		}
	}

	// region basic tests

	TEST(TEST_CLASS, FailureWhenTransactionDeadlineIsLessThanBlockTime) {
		// Assert:
		for (auto i = 0u; i < 4; ++i)
			AssertValidationResult(Failure_Core_Past_Deadline, Block_Time - Timestamp(1), TimeSpanFromHours(i));
	}

	TEST(TEST_CLASS, SuccessWhenTransactionDeadlineIsLessThanBlockTimeAndValidationDisabled) {
		// Assert:
		for (auto i = 0u; i < 4; ++i)
			AssertValidationResult(ValidationResult::Success, Block_Time - Timestamp(1),
								   TimeSpanFromHours(i), false /* enableDeadlineValidation */);
	}

	TEST(TEST_CLASS, SuccessWhenTransactionDeadlineIsEqualToBlockTime) {
		// Assert:
		for (auto i = 0u; i < 4; ++i)
			AssertValidationResult(ValidationResult::Success, Block_Time, TimeSpanFromHours(i));
	}

	TEST(TEST_CLASS, SuccessWhenTransactionDeadlineIsValid) {
		// Assert:
		for (auto i = 0u; i < 4; ++i)
			AssertValidationResult(ValidationResult::Success, Block_Time + utils::TimeSpan::FromMinutes(30), TimeSpanFromHours(i));
	}

	TEST(TEST_CLASS, SuccessWhenTransactionDeadlineIsEqualToBlockTimePlusLifetime) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, Block_Time + TimeSpanFromHours(2), utils::TimeSpan());

		for (auto i = 1u; i < 4; ++i)
			AssertValidationResult(ValidationResult::Success, Block_Time + TimeSpanFromHours(i), TimeSpanFromHours(i));
	}

	TEST(TEST_CLASS, FailureWhenTransactionDeadlineIsGreaterThanBlockTimePlusLifetime) {
		// Assert:
		AssertValidationResult(Failure_Core_Future_Deadline, Block_Time + TimeSpanFromHours(3), utils::TimeSpan());

		for (auto i = 1u; i < 4; ++i)
			AssertValidationResult(Failure_Core_Future_Deadline, Block_Time + TimeSpanFromHours(i + 1), TimeSpanFromHours(i));
	}

	// endregion
}}
