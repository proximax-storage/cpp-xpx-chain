/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/NetworkConfigCache.h"
#include "src/cache/NetworkConfigCacheStorage.h"
#include "src/config/NetworkConfigConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/NetworkConfigTestUtils.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS NetworkConfigValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(NetworkConfig, plugins::PluginManager(config::CreateMockConfigurationHolder(), plugins::StorageConfiguration()))

	namespace {
		std::string commonBlockChainProperties{
			"[network]\n"
			"\n"
			"publicKey = B4F12E7C9F6946091E2CB8B6D3A12B50D17CCBBF646386EA27CE2946A7423DCF\n"
			"\n"
			"[chain]\n"
			"\n"
			"blockGenerationTargetTime = 15s\n"
			"blockTimeSmoothingFactor = 3000\n"
			"\n"
			"greedDelta = 0.5\n"
			"greedExponent = 2\n"
			"\n"
			"importanceGrouping = 5760\n"
			"maxRollbackBlocks = 360\n"
			"maxDifficultyBlocks = 3\n"
			"\n"
			"maxTransactionLifetime = 24h\n"
			"maxBlockFutureTime = 10s\n"
			"\n"
			"maxMosaicAtomicUnits = 9'000'000'000'000'000\n"
			"\n"
			"totalChainImportance = 8'999'999'998'000'000\n"
			"minHarvesterBalance = 1'000'000'000'000\n"
			"harvestBeneficiaryPercentage = 10\n"
			"\n"
			"blockPruneInterval = 360\n"
			"maxTransactionsPerBlock = 200'000\n\n"
		};

		std::string pluginConfigs{
			"[plugin:catapult.plugins.config]\n"
			"\n"
			"maxBlockChainConfigSize = 1MB\n"
			"maxSupportedEntityVersionsSize = 1MB\n"
		};

		std::string networkConfigWithPlugin{commonBlockChainProperties + pluginConfigs};

		std::shared_ptr<plugins::PluginManager> CreatePluginManager(uint64_t maxBlockChainConfigSizeMb, uint64_t maxSupportedEntityVersionsSizeMb) {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::NetworkConfigConfiguration::Uninitialized();
			pluginConfig.MaxBlockChainConfigSize = utils::FileSize::FromMegabytes(maxBlockChainConfigSizeMb);
			pluginConfig.MaxSupportedEntityVersionsSize = utils::FileSize::FromMegabytes(maxSupportedEntityVersionsSizeMb);
			config.Network.SetPluginConfiguration(PLUGIN_NAME(config), pluginConfig);
			config.Network.Plugins.emplace(PLUGIN_NAME(config), utils::ConfigurationBag({}));
			auto pConfigHolder = config::CreateMockConfigurationHolder(config.ToConst());
			return std::make_shared<plugins::PluginManager>(pConfigHolder, plugins::StorageConfiguration());
		}

		std::shared_ptr<plugins::PluginManager> CreatePluginManagerWithRealPlugins(uint64_t maxBlockChainConfigSizeMb, uint64_t maxSupportedEntityVersionsSizeMb) {
			test::MutableBlockchainConfiguration config;
			std::istringstream input(networkConfigWithPlugin);
			config.Network = model::NetworkConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(input));
			auto pluginConfig = config::NetworkConfigConfiguration::Uninitialized();
			pluginConfig.MaxBlockChainConfigSize = utils::FileSize::FromMegabytes(maxBlockChainConfigSizeMb);
			pluginConfig.MaxSupportedEntityVersionsSize = utils::FileSize::FromMegabytes(maxSupportedEntityVersionsSizeMb);
			config.Network.SetPluginConfiguration(PLUGIN_NAME(config), pluginConfig);
			return test::CreatePluginManagerWithRealPlugins(config.Network);
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const std::string& networkConfig,
				const std::string& supportedEntityVersions,
				std::shared_ptr<plugins::PluginManager> pPluginManager,
				cache::CatapultCache& cache) {
			// Arrange:
			model::NetworkConfigNotification<1> notification(
				BlockDuration(),
				networkConfig.size(),
				reinterpret_cast<const uint8_t*>(networkConfig.data()),
				supportedEntityVersions.size(),
				reinterpret_cast<const uint8_t*>(supportedEntityVersions.data()));
			auto pValidator = CreateNetworkConfigValidator(*pPluginManager);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, Height(1));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		void RunTest(
				ValidationResult expectedResult,
				const std::string& networkConfig,
				const std::string& supportedEntityVersions,
				uint64_t maxBlockChainConfigSizeMb = 1,
				uint64_t maxSupportedEntityVersionsSizeMb = 1,
				bool seedConfigCache = false) {
			auto pPluginManager = CreatePluginManager(maxBlockChainConfigSizeMb, maxSupportedEntityVersionsSizeMb);
			auto cache = test::CreateEmptyCatapultCache<test::NetworkConfigCacheFactory>();
			if (seedConfigCache) {
				auto delta = cache.createDelta();
				auto& configCacheDelta = delta.sub<cache::NetworkConfigCache>();
				configCacheDelta.insert(state::NetworkConfigEntry(Height(1), "BlockChainConfig", "SupportedEntityVersionsConfig"));
				cache.commit(Height(1));
			}
			AssertValidationResult(expectedResult, networkConfig, supportedEntityVersions, pPluginManager, cache);
		}

		void RunTestWithRealPlugins(
				ValidationResult expectedResult,
				const std::string& networkConfig,
				const std::string& supportedEntityVersions,
				uint64_t maxBlockChainConfigSizeMb = 1,
				uint64_t maxSupportedEntityVersionsSizeMb = 1) {
			auto pPluginManager = CreatePluginManagerWithRealPlugins(maxBlockChainConfigSizeMb, maxSupportedEntityVersionsSizeMb);
			auto cache = pPluginManager->createCache();
			AssertValidationResult(expectedResult, networkConfig, supportedEntityVersions, pPluginManager, cache);
		}
	}

	TEST(TEST_CLASS, FailureWhenBlockChainConfigTooBig) {
		// Assert:
		RunTest(
			Failure_NetworkConfig_BlockChain_Config_Too_Large,
			networkConfigWithPlugin,
			test::GetSupportedEntityVersionsString(),
			0,
			1);
	}

	TEST(TEST_CLASS, FailureWhenSupportedEntityVersionsConfigTooBig) {
		// Assert:
		RunTest(
			Failure_NetworkConfig_SupportedEntityVersions_Config_Too_Large,
			networkConfigWithPlugin,
			test::GetSupportedEntityVersionsString(),
			1,
			0);
	}

	TEST(TEST_CLASS, FailureNetworkConfigExistsAtHeight) {
		// Assert:
		RunTest(
			Failure_NetworkConfig_Config_Redundant,
			networkConfigWithPlugin,
			test::GetSupportedEntityVersionsString(),
			1,
			1,
			true);
	}

	TEST(TEST_CLASS, FailureWhenBlockChainConfigInvalid) {
		// Assert:
		RunTest(
			Failure_NetworkConfig_BlockChain_Config_Malformed,
			"invalidConfig",
			test::GetSupportedEntityVersionsString());
	}

	TEST(TEST_CLASS, FailureWhenPluginConfigMissing) {
		// Assert:
		RunTest(
			Failure_NetworkConfig_Plugin_Config_Missing,
			commonBlockChainProperties,
			test::GetSupportedEntityVersionsString());
	}

	TEST(TEST_CLASS, FailureWhenPluginConfigInvalid) {
		// Assert:
		auto networkConfig = commonBlockChainProperties +
			"[plugin:catapult.plugins.config]\n\n"
			"qwerty = 12";
		RunTestWithRealPlugins(
			Failure_NetworkConfig_Plugin_Config_Malformed,
			networkConfig,
			test::GetSupportedEntityVersionsString());
	}

	TEST(TEST_CLASS, FailureWhenNetworkConfigTransactionHasNoSupportedVersions) {
		// Assert:
		RunTest(
			Failure_NetworkConfig_Network_Config_Trx_Cannot_Be_Unsupported,
			networkConfigWithPlugin,
			"{\n"
			"\t\"entities\": [\n"
			"\t\t{\n"
			"\t\t\t\"name\": \"Mosaic_Definition\",\n"
			"\t\t\t\"type\": \"16717\",\n"
			"\t\t\t\"supportedVersions\": [3]\n"
			"\t\t},\n"
			"\t\t{\n"
			"\t\t\t\"name\": \"Mosaic_Supply_Change\",\n"
			"\t\t\t\"type\": \"16973\",\n"
			"\t\t\t\"supportedVersions\": [2]\n"
			"\t\t},\n"
			"\t\t{\n"
			"\t\t\t\"name\": \"Account_Link\",\n"
			"\t\t\t\"type\": \"16716\",\n"
			"\t\t\t\"supportedVersions\": [2]\n"
			"\t\t}\n"
			"\t]\n"
			"}");
	}

	TEST(TEST_CLASS, FailureWhenSupportedEntityVersionsConfigInvalid) {
		// Assert:
		RunTest(
			Failure_NetworkConfig_SupportedEntityVersions_Config_Malformed,
			networkConfigWithPlugin,
			"invalidConfig");
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		RunTestWithRealPlugins(
			ValidationResult::Success,
			networkConfigWithPlugin,
			test::GetSupportedEntityVersionsString());
	}
}}
