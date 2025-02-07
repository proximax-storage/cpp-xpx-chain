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
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"
#include <src/config/StreamingConfiguration.h>
#include "catapult/model/StreamingNotifications.h"

namespace catapult { namespace validators {

#define TEST_CLASS StreamStartFolderNameValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(StreamStartFolderName)

	namespace {
		void AssertValidationResult(ValidationResult expectedResult, uint16_t folderNameSize, uint16_t maxFolderNameSize) {
			// Arrange:
			auto notification = model::StreamStartFolderNameNotification<1>(folderNameSize);
			auto pluginConfig = config::StreamingConfiguration::Uninitialized();
			pluginConfig.MaxFolderNameSize = maxFolderNameSize;
			test::MutableBlockchainConfiguration mutableConfig;
			mutableConfig.Network.SetPluginConfiguration(pluginConfig);
			auto config = mutableConfig.ToConst();
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pValidator = CreateStreamStartFolderNameValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, config);

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationWithFolderNameSizeLessThanMax) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, 512, 513);
	}

	TEST(TEST_CLASS, SuccessWhenValidatingNotificationWithFolderNameSizeEqualToMax) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, 512, 512);
	}

	TEST(TEST_CLASS, FailureWhenValidatingNotificationWithFolderNameSizeGreaterThanMax) {
		// Assert:
		AssertValidationResult(Failure_Streaming_Folder_Name_Too_Large, 512, 511);
	}
}}
