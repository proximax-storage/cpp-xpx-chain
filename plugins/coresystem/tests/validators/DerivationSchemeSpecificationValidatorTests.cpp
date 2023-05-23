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

#define TEST_CLASS DerivationSchemeSpecificationValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(DerivationSchemeSpecification)

	namespace {
		constexpr uint8_t Min_Entity_Version = 55;
		constexpr auto Entity_Type = model::EntityType{1};

		auto CreateBlockchainConfigurationHolder() {
			auto pConfigHolder = config::CreateMockConfigurationHolder();
			const_cast<config::SupportedEntityVersions&>(pConfigHolder->Config().SupportedEntityVersions)[Entity_Type].emplace(Min_Entity_Version);
			return pConfigHolder;
		}

		void AssertValidationResult(ValidationResult expectedResult, uint accountVersion, DerivationScheme derivationScheme) {
			// Arrange:
			auto cache = test::CreateEmptyCatapultCache();
			auto cacheView = cache.createView();
			auto readOnlyCache = cacheView.toReadOnly();
			auto resolverContext = test::CreateResolverContextXor();
			auto pConfigHolder = CreateBlockchainConfigurationHolder();
			const_cast<model::NetworkConfiguration&>(pConfigHolder->Config().Network).AccountVersion = accountVersion;
			auto context = ValidatorContext(pConfigHolder->Config(), Height(123), Timestamp(8888), resolverContext, readOnlyCache);
			model::EntityNotification<1> notification(model::NetworkIdentifier::Zero, Entity_Type, Min_Entity_Version, derivationScheme);
			auto pValidator = CreateDerivationSchemeSpecificationValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, context);

			// Assert:
			EXPECT_EQ(expectedResult, result) << "account " << (accountVersion == 1u ? "default" : "active");
		}
	}

	// region validation

	TEST(TEST_CLASS, FailureWhenDerivationSchemeIsUnsetAndAccountVersionIsBeingUsed) {
		// Assert:
		AssertValidationResult(Failure_Core_Derivation_Scheme_Unset, 2, DerivationScheme::Unset);

	}

	TEST(TEST_CLASS, SuccessWhenDerivationSchemeIsSetAndAccountVersionIsBeingUsed) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, 2, DerivationScheme::Ed25519_Sha3);

	}

	TEST(TEST_CLASS, SuccessWhenDerivationSchemeIsUnsetAndAccountVersionIsDefault) {
		// Assert:
		AssertValidationResult(ValidationResult::Success, 1, DerivationScheme::Unset);

	}

	// endregion
}}
