/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/BlockchainUpgradeCache.h"
#include "src/cache/BlockchainUpgradeCacheStorage.h"
#include "src/config/BlockchainUpgradeConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/BlockchainUpgradeTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS BlockchainUpgradeValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(BlockchainUpgrade)

	namespace {
		auto CreateConfigHolder(uint64_t minUpgradePeriod) {
			test::MutableBlockchainConfiguration config;
			auto pluginUpgrade = config::BlockchainUpgradeConfiguration::Uninitialized();
			pluginUpgrade.MinUpgradePeriod = BlockDuration{minUpgradePeriod};
			config.Network.SetPluginConfiguration(pluginUpgrade);
			return config::CreateMockConfigurationHolder(config.ToConst());
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				uint64_t minUpgradePeriod,
				uint64_t upgradePeriod,
				uint64_t nextBlockchainVersion,
				bool seedCache = false) {
			// Arrange:
			auto cache = test::CreateEmptyCatapultCache<test::BlockchainUpgradeCacheFactory>();
			if (seedCache) {
				auto delta = cache.createDelta();
				auto& upgradeCacheDelta = delta.sub<cache::BlockchainUpgradeCache>();
				upgradeCacheDelta.insert(state::BlockchainUpgradeEntry(Height(1 + upgradePeriod), BlockchainVersion{100}));
				cache.commit(Height(1));
			}
			model::BlockchainUpgradeVersionNotification<1> notification(BlockDuration{upgradePeriod}, BlockchainVersion{nextBlockchainVersion});
			auto pValidator = CreateBlockchainUpgradeValidator();

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, CreateConfigHolder(minUpgradePeriod)->Config(), Height(1));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}
	}

	TEST(TEST_CLASS, FailureWhenUpgradePeriodTooLow) {
		// Assert:
		AssertValidationResult(
			Failure_BlockchainUpgrade_Upgrade_Period_Too_Low,
			100,
			50,
			std::numeric_limits<uint64_t>::max());
	}

	TEST(TEST_CLASS, FailureBlockchainUpgradeExistsAtHeight) {
		// Assert:
		AssertValidationResult(
			Failure_BlockchainUpgrade_Redundant,
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
