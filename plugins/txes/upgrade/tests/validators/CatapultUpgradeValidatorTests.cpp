/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/CatapultUpgradeCache.h"
#include "src/cache/CatapultUpgradeCacheStorage.h"
#include "src/config/CatapultUpgradeConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/CatapultUpgradeTestUtils.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/other/MutableCatapultConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS CatapultUpgradeValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(CatapultUpgrade, config::CreateMockConfigurationHolder())

	namespace {
		auto CreateConfigHolder(uint64_t minUpgradePeriod) {
			test::MutableCatapultConfiguration config;
			auto pluginUpgrade = config::CatapultUpgradeConfiguration::Uninitialized();
			pluginUpgrade.MinUpgradePeriod = BlockDuration{minUpgradePeriod};
			config.BlockChain.SetPluginConfiguration(PLUGIN_NAME(upgrade), pluginUpgrade);
			return config::CreateMockConfigurationHolder(config.ToConst());
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				uint64_t minUpgradePeriod,
				uint64_t upgradePeriod,
				uint64_t nextCatapultVersion,
				bool seedCache = false) {
			// Arrange:
			auto cache = test::CreateEmptyCatapultCache<test::CatapultUpgradeCacheFactory>(model::BlockChainConfiguration::Uninitialized());
			if (seedCache) {
				auto delta = cache.createDelta();
				auto& upgradeCacheDelta = delta.sub<cache::CatapultUpgradeCache>();
				upgradeCacheDelta.insert(state::CatapultUpgradeEntry(Height(1 + upgradePeriod), CatapultVersion{100}));
				cache.commit(Height(1));
			}
			model::CatapultUpgradeVersionNotification<1> notification(BlockDuration{upgradePeriod}, CatapultVersion{nextCatapultVersion});
			auto pValidator = CreateCatapultUpgradeValidator(CreateConfigHolder(minUpgradePeriod));

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, Height(1));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenUpgradePeriodTooLow) {
		// Assert:
		AssertValidationResult(
			Failure_CatapultUpgrade_Upgrade_Period_Too_Low,
			100,
			50,
			std::numeric_limits<uint64_t>::max());
	}

	TEST(TEST_CLASS, FailureFutureCatapultVersionTooLow) {
		// Assert:
		AssertValidationResult(
			Failure_CatapultUpgrade_Catapult_Version_Lower_Than_Current,
			10,
			10,
			0);
	}

	TEST(TEST_CLASS, FailureCatapultUpgradeExistsAtHeight) {
		// Assert:
		AssertValidationResult(
			Failure_CatapultUpgrade_Redundant,
			5,
			20,
			std::numeric_limits<uint64_t>::max(),
			true);
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		AssertValidationResult(
			ValidationResult::Success,
			5,
			10,
			std::numeric_limits<uint64_t>::max());
	}
}}
