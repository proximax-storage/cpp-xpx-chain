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
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS NetworkValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(Network, std::make_shared<config::LocalNodeConfigurationHolder>())

	namespace {
		constexpr auto Network_Identifier = static_cast<model::NetworkIdentifier>(123);

		void AssertValidationResult(ValidationResult expectedResult, uint8_t networkIdentifier) {
			// Arrange:
			model::EntityNotification<1> notification(static_cast<model::NetworkIdentifier>(networkIdentifier), model::EntityType{0}, 0);
			auto config = model::BlockChainConfiguration::Uninitialized();
			config.Network.Identifier = Network_Identifier;
			auto cache = test::CreateEmptyCatapultCache(config);
			auto pConfigHolder = std::make_shared<config::LocalNodeConfigurationHolder>();
			pConfigHolder->SetBlockChainConfig(Height{0}, config);
			auto pValidator = CreateNetworkValidator(pConfigHolder);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "network identifier " << networkIdentifier;
		}
	}

	// region validation

	TEST(TEST_CLASS, SuccessWhenEntityHasSpecifiedCorrectNetwork) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, 123);
	}

	TEST(TEST_CLASS, FailureWhenEntityHasSpecifiedIncorrectNetwork) {
		// Assert:
		for (uint8_t identifier = 0; identifier < 255; ++identifier) {
			if (123u == identifier)
				continue;

			AssertValidationResult(Failure_Core_Wrong_Network, identifier);
		}
	}

	// endregion
}}
