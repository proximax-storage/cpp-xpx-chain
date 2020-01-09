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
#include "src/cache/NamespaceCache.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/NamespaceCacheTestUtils.h"
#include "tests/test/NamespaceTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS RootNamespaceMaxChildrenValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(RootNamespaceMaxChildren)

	namespace {
		auto CreateAndSeedCache() {
			auto cache = test::NamespaceCacheFactory::Create();
			{
				auto cacheDelta = cache.createDelta();
				auto& namespaceCacheDelta = cacheDelta.sub<cache::NamespaceCache>();
				auto rootOwner = test::GenerateRandomByteArray<Key>();

				namespaceCacheDelta.insert(state::RootNamespace(NamespaceId(25), rootOwner, test::CreateLifetime(10, 20)));
				namespaceCacheDelta.insert(state::Namespace(test::CreatePath({ 25, 36 })));
				namespaceCacheDelta.insert(state::Namespace(test::CreatePath({ 25, 36, 49 })));
				namespaceCacheDelta.insert(state::Namespace(test::CreatePath({ 25, 37 })));

				// Sanity:
				test::AssertCacheContents(namespaceCacheDelta, { 25, 36, 49, 37 });

				cache.commit(Height());
			}

			return cache;
		}

		void RunTest(ValidationResult expectedResult, const model::ChildNamespaceNotification<1>& notification, uint16_t maxChildren) {
			// Arrange:
			auto cache = CreateAndSeedCache();
			auto pluginConfig = config::NamespaceConfiguration::Uninitialized();
			pluginConfig.MaxChildNamespaces = maxChildren;
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.SetPluginConfiguration(pluginConfig);
			auto pConfigHolder = config::CreateMockConfigurationHolder(networkConfig);
			auto pValidator = CreateRootNamespaceMaxChildrenValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, pConfigHolder->Config());

			// Assert:
			EXPECT_EQ(expectedResult, result) << "maxChildren " << maxChildren;
		}
	}

	TEST(TEST_CLASS, FailureWhenMaxChildrenIsExceeded) {
		// Act: root with id 25 has 3 children
		auto notification = model::ChildNamespaceNotification<1>(Key(), NamespaceId(26), NamespaceId(25));
		RunTest(Failure_Namespace_Max_Children_Exceeded, notification, 1);
		RunTest(Failure_Namespace_Max_Children_Exceeded, notification, 2);
		RunTest(Failure_Namespace_Max_Children_Exceeded, notification, 3);
	}

	TEST(TEST_CLASS, SuccessWhenMaxChildrenIsNotExceeded) {
		// Act: root with id 25 has 3 children
		auto notification = model::ChildNamespaceNotification<1>(Key(), NamespaceId(26), NamespaceId(25));
		RunTest(ValidationResult::Success, notification, 4);
		RunTest(ValidationResult::Success, notification, 5);
		RunTest(ValidationResult::Success, notification, 123);
	}
}}
