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

#include "src/config/NamespaceConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS RootNamespaceValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(RootNamespace, config::CreateMockConfigurationHolder())

	// region duration

	namespace {
		void AssertDurationValidationResult(ValidationResult expectedResult, uint16_t duration, uint16_t maxDuration) {
			// Arrange:
			auto pluginConfig = config::NamespaceConfiguration::Uninitialized();
			pluginConfig.MaxNamespaceDuration = utils::BlockSpan::FromHours(maxDuration);
			auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
			blockChainConfig.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
			blockChainConfig.SetPluginConfiguration(PLUGIN_NAME(namespace), pluginConfig);
			auto cache = test::CreateEmptyCatapultCache(blockChainConfig);
			auto pConfigHolder = config::CreateMockConfigurationHolder(blockChainConfig);
			auto pValidator = CreateRootNamespaceValidator(pConfigHolder);
			auto notification = model::RootNamespaceNotification<1>(Key(), NamespaceId(), BlockDuration(duration));

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "duration " << duration << ", maxDuration " << maxDuration;
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingRootNamespaceWithDurationLessThanMax) {
		// Assert:
		AssertDurationValidationResult(ValidationResult::Success, 12312, 12345);
	}

	TEST(TEST_CLASS, SuccessWhenValidatingRootNamespaceWithDurationEqualToMax) {
		// Assert:
		AssertDurationValidationResult(ValidationResult::Success, 12345, 12345);
	}

	TEST(TEST_CLASS, FailureWhenValidatingRootNamespaceWithDurationGreaterThanMax) {
		// Assert:
		AssertDurationValidationResult(Failure_Namespace_Invalid_Duration, 12346, 12345);
		AssertDurationValidationResult(Failure_Namespace_Invalid_Duration, 65432, 12345);
	}

	TEST(TEST_CLASS, SuccessWhenValidatingRootNamespaceWithZeroDuration) {
		// Assert: eternal duration is allowed
		AssertDurationValidationResult(ValidationResult::Success, 0, 12345);
	}

	// endregion
}}
