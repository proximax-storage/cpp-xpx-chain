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

#include "src/config/TransferConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS TransferMessageValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(TransferMessage, config::CreateMockConfigurationHolder())

	namespace {
		void AssertValidationResult(ValidationResult expectedResult, uint16_t messageSize, uint16_t maxMessageSize) {
			// Arrange:
			auto notification = model::TransferMessageNotification<1>(messageSize);
			auto pluginConfig = config::TransferConfiguration::Uninitialized();
			pluginConfig.MaxMessageSize = maxMessageSize;
			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Network.SetPluginConfiguration(PLUGIN_NAME(transfer), pluginConfig);
			auto config = mutableConfig.ToConst();
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config);
			auto pValidator = CreateTransferMessageValidator(pConfigHolder);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationWithMessageSizeLessThanMax) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, 100, 1234);
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationWithMessageSizeEqualToMax) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, 1234, 1234);
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationWithMessageSizeGreaterThanMax) {
		// Assert:
		AssertValidationResult(Failure_Transfer_Message_Too_Large, 1235, 1234);
	}
}}
