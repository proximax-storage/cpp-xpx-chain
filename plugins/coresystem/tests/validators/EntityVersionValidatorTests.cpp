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
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS EntityVersionValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(EntityVersion, config::CreateMockConfigurationHolder())

	namespace {
		constexpr uint8_t Min_Entity_Version = 55;
		constexpr uint8_t Max_Entity_Version = 77;
		constexpr auto Entity_Type = model::EntityType{1};

		auto CreateBlockchainConfigurationHolder() {
			auto pConfigHolder = config::CreateMockConfigurationHolder();
			for (uint16_t i = Min_Entity_Version; i <= Max_Entity_Version; ++i)
				const_cast<config::SupportedEntityVersions&>(pConfigHolder->Config().SupportedEntityVersions)[Entity_Type].emplace(i);
			return pConfigHolder;
		}

		void AssertValidationResult(ValidationResult expectedResult, uint8_t version) {
			// Arrange:
			auto config = model::NetworkConfiguration::Uninitialized();
			auto cache = test::CreateEmptyCatapultCache(config);
			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto resolverContext = test::CreateResolverContextXor();
			auto context = ValidatorContext(Height(123), Timestamp(8888), model::NetworkInfo(), resolverContext, readOnlyCache);
			model::EntityNotification<1> notification(model::NetworkIdentifier::Zero, Entity_Type, version);
			auto pValidator = CreateEntityVersionValidator(CreateBlockchainConfigurationHolder());

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "entity version " << static_cast<uint16_t>(version);
		}
	}

	// region validation

	TEST(TEST_CLASS, FailureWhenEntityHasVersionLowerThanMinVersion) {
		// Assert:
		for (uint8_t version = 0u; version < Min_Entity_Version; ++version)
			AssertValidationResult(Failure_Core_Invalid_Version, version);
	}

	TEST(TEST_CLASS, FailureWhenEntityHasVersionGreaterThanMaxVersion) {
		// Assert:
		for (uint8_t version = Max_Entity_Version + 1; 0 != version; ++version)
			AssertValidationResult(Failure_Core_Invalid_Version, version);
	}

	TEST(TEST_CLASS, SuccessWhenEntityHasVersionWithinBounds) {
		// Assert:
		for (uint8_t version = Min_Entity_Version; version <= Max_Entity_Version; ++version)
			AssertValidationResult(ValidationResult::Success, version);
	}

	// endregion
}}
