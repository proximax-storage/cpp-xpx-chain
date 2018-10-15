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

#include "catapult/config/ValidateConfiguration.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/utils/ConfigurationBag.h"
#include "tests/test/net/NodeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace config {

#define TEST_CLASS ValidateConfigurationTests

	namespace {
		// the key is invalid because it contains a non hex char ('G')
		const char* Invalid_Private_Key = "3485D98EFD7EB07ABAFCFD1A157D89DE2G96A95E780813C0258AF3F5F84ED8CB";

		auto CreateValidNodeConfiguration() {
			return NodeConfiguration::Uninitialized();
		}

		auto CreateAndValidateLocalNodeConfiguration(
				UserConfiguration&& userConfig,
				NodeConfiguration&& nodeConfig = CreateValidNodeConfiguration()) {
			// Act:
			auto config = LocalNodeConfiguration(
					model::BlockChainConfiguration::Uninitialized(),
					std::move(nodeConfig),
					LoggingConfiguration::Uninitialized(),
					std::move(userConfig));
			ValidateConfiguration(config);
		}
	}

	// region boot key validation

	namespace {
		void AssertInvalidBootKey(const std::string& bootKey) {
			// Arrange:
			auto userConfig = UserConfiguration::Uninitialized();
			userConfig.BootKey = bootKey;

			// Act + Assert:
			EXPECT_THROW(CreateAndValidateLocalNodeConfiguration(std::move(userConfig)), utils::property_malformed_error);
		}
	}

	TEST(TEST_CLASS, ValidationFailsIfBootKeyIsInvalid) {
		// Assert:
		AssertInvalidBootKey(Invalid_Private_Key);
		AssertInvalidBootKey("");
	}

	// endregion
}}
