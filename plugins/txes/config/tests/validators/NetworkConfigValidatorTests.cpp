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
#include <boost/algorithm/string/replace.hpp>

namespace catapult { namespace validators {

#define TEST_CLASS NetworkConfigValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(NetworkConfig, plugins::PluginManager(config::CreateMockConfigurationHolder(), plugins::StorageConfiguration()))
	DEFINE_COMMON_VALIDATOR_TESTS(NetworkConfigV2, plugins::PluginManager(config::CreateMockConfigurationHolder(), plugins::StorageConfiguration()))

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
			"accountVersion = 1\n"
			"minimumAccountVersion = 1\n"
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
			"enableUnconfirmedTransactionMinFeeValidation = true\n\n"
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
			config.Immutable.InitialCurrencyAtomicUnits = Amount(100);
			auto pluginConfig = config::NetworkConfigConfiguration::Uninitialized();
			pluginConfig.MaxBlockChainConfigSize = utils::FileSize::FromMegabytes(maxBlockChainConfigSizeMb);
			pluginConfig.MaxSupportedEntityVersionsSize = utils::FileSize::FromMegabytes(maxSupportedEntityVersionsSizeMb);
			config.Network.SetPluginConfiguration(pluginConfig);
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
			config.Network.SetPluginConfiguration(pluginConfig);
			return test::CreatePluginManagerWithRealPlugins(config.Network);
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const std::string& networkConfig,
				const std::string& supportedEntityVersions,
				std::shared_ptr<plugins::PluginManager> pPluginManager,
				cache::CatapultCache& cache,
				BlockDuration applyHeightDelta = BlockDuration(10)) {
			// Arrange:
			model::NetworkConfigNotification<1> notification(
				applyHeightDelta,
				networkConfig.size(),
				reinterpret_cast<const uint8_t*>(networkConfig.data()),
				supportedEntityVersions.size(),
				reinterpret_cast<const uint8_t*>(supportedEntityVersions.data()));
			auto pValidator = CreateNetworkConfigValidator(*pPluginManager);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, pPluginManager->configHolder()->Config(), Height(1));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const std::string& networkConfig,
				const std::string& supportedEntityVersions,
				std::shared_ptr<plugins::PluginManager> pPluginManager,
				cache::CatapultCache& cache,
				model::NetworkUpdateType Type,
				uint64_t applyHeightDelta = 10) {
			// Arrange:
			model::NetworkConfigNotification<2> notification(
					Type,
					applyHeightDelta,
					networkConfig.size(),
					reinterpret_cast<const uint8_t*>(networkConfig.data()),
					supportedEntityVersions.size(),
					reinterpret_cast<const uint8_t*>(supportedEntityVersions.data()));
			auto pValidator = CreateNetworkConfigV2Validator(*pPluginManager);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, pPluginManager->configHolder()->Config(), Height(1));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		template<uint32_t TVersion>
		void RunTest(
				ValidationResult expectedResult,
				const std::string& networkConfig,
				const std::string& supportedEntityVersions,
				model::NetworkUpdateType type,
				uint64_t maxBlockChainConfigSizeMb = 1,
				uint64_t maxSupportedEntityVersionsSizeMb = 1,
				bool seedConfigCache = false,
				uint64_t applyHeightDelta = 10) {
			auto pPluginManager = CreatePluginManager(maxBlockChainConfigSizeMb, maxSupportedEntityVersionsSizeMb);
			auto cache = test::CreateEmptyCatapultCache<test::NetworkConfigCacheFactory>();
			if (seedConfigCache) {
				auto delta = cache.createDelta();
				auto& configCacheDelta = delta.sub<cache::NetworkConfigCache>();
				if constexpr(TVersion == 1)
					configCacheDelta.insert(state::NetworkConfigEntry(Height(1 + applyHeightDelta), networkConfig, supportedEntityVersions));
				else {
					if(type == model::NetworkUpdateType::Delta)
						configCacheDelta.insert(state::NetworkConfigEntry(Height(1 + applyHeightDelta), networkConfig, supportedEntityVersions));
					else
						configCacheDelta.insert(state::NetworkConfigEntry(Height(applyHeightDelta), networkConfig, supportedEntityVersions));
				}
				cache.commit(Height(1));
			}
			if constexpr(TVersion == 1)
				AssertValidationResult(expectedResult, networkConfig, supportedEntityVersions, pPluginManager, cache, BlockDuration(applyHeightDelta));
			else
				AssertValidationResult(expectedResult, networkConfig, supportedEntityVersions, pPluginManager, cache, type, applyHeightDelta);


		}
		template<uint32_t TVersion>
		void RunTestWithRealPlugins(
				ValidationResult expectedResult,
				const std::string& networkConfig,
				const std::string& supportedEntityVersions,
				model::NetworkUpdateType type,
				uint64_t maxBlockChainConfigSizeMb = 1,
				uint64_t maxSupportedEntityVersionsSizeMb = 1) {
			auto pPluginManager = CreatePluginManagerWithRealPlugins(maxBlockChainConfigSizeMb, maxSupportedEntityVersionsSizeMb);
			auto cache = pPluginManager->createCache();
			if constexpr(TVersion == 1)
				AssertValidationResult(expectedResult, networkConfig, supportedEntityVersions, pPluginManager, cache);
			else
				AssertValidationResult(expectedResult, networkConfig, supportedEntityVersions, pPluginManager, cache, type);
		}
	}
#define TRAITS_BASED_TEST(TEST_NAME) \
	template<uint32_t TVersion, model::NetworkUpdateType TType = model::NetworkUpdateType::Delta> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_V1) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<1>(); } \
	TEST(TEST_CLASS, TEST_NAME##_V2_Delta) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<2>(); } \
	TEST(TEST_CLASS, TEST_NAME##_V2_Absolute) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<2, model::NetworkUpdateType::Absolute>(); } \
	template<uint32_t TVersion, model::NetworkUpdateType TType> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	TRAITS_BASED_TEST(FailureWhenApplyHeightDeltaIsZero) {
		// Assert:
		RunTest<TVersion>(
			TType == model::NetworkUpdateType::Delta ? Failure_NetworkConfig_ApplyHeightDelta_Zero : Failure_NetworkConfig_ApplyHeight_In_The_Past,
			networkConfigWithPlugin,
			test::GetSupportedEntityVersionsString(),
			TType,
			1,
			1,
			false,
			0);
	}

	TRAITS_BASED_TEST(FailureWhenBlockChainConfigTooBig) {
		// Assert:
		RunTest<TVersion>(
			Failure_NetworkConfig_BlockChain_Config_Too_Large,
			networkConfigWithPlugin,
			test::GetSupportedEntityVersionsString(),
			TType,
			0,
			1);
	}

	TRAITS_BASED_TEST(FailureWhenSupportedEntityVersionsConfigTooBig) {
		// Assert:
		RunTest<TVersion>(
			Failure_NetworkConfig_SupportedEntityVersions_Config_Too_Large,
			networkConfigWithPlugin,
			test::GetSupportedEntityVersionsString(),
			TType,
			1,
			0);
	}

	TRAITS_BASED_TEST(FailureWhenNetworkConfigExistsAtHeight) {
		// Assert:
		RunTest<TVersion>(
			Failure_NetworkConfig_Config_Redundant,
			networkConfigWithPlugin,
			test::GetSupportedEntityVersionsString(),
			TType,
			1,
			1,
			true);
	}

	TRAITS_BASED_TEST(FailureWhenImportanceGroupingInvalid) {
		auto networkConfig = networkConfigWithPlugin;
		boost::algorithm::replace_first(networkConfig, "importanceGrouping = 5760", "importanceGrouping = 100");
		// Assert:
		RunTest<TVersion>(
			Failure_NetworkConfig_ImportanceGrouping_Less_Or_Equal_Half_MaxRollbackBlocks,
			networkConfig,
			test::GetSupportedEntityVersionsString(),
			TType);
	}

	TRAITS_BASED_TEST(FailureWhenAccountVersionIsLowerThanCurrentVersion) {
		auto networkConfig = networkConfigWithPlugin;
		boost::algorithm::replace_first(networkConfig, "accountVersion = 1", "accountVersion = 0");
		// Assert:
		RunTest<TVersion>(
				Failure_NetworkConfig_AccountVersion_Less_Than_Current,
				networkConfig,
				test::GetSupportedEntityVersionsString(),
				TType);
	}

	TRAITS_BASED_TEST(FailureWhenMinimumAccountVersionIsLowerThanCurrentVersion) {
		auto networkConfig = networkConfigWithPlugin;
		boost::algorithm::replace_first(networkConfig, "minimumAccountVersion = 1", "minimumAccountVersion = 0");
		// Assert:
		RunTest<TVersion>(
				Failure_NetworkConfig_MinimumAccountVersion_Less_Than_Current,
				networkConfig,
				test::GetSupportedEntityVersionsString(),
				TType);
	}

	TRAITS_BASED_TEST(FailureWhenAccountVersionIsLowerThanMinimum) {
		auto networkConfig = networkConfigWithPlugin;
		boost::algorithm::replace_first(networkConfig, "accountVersion = 1", "accountVersion = 1");
		boost::algorithm::replace_first(networkConfig, "minimumAccountVersion = 1", "minimumAccountVersion = 2");
		// Assert:
		RunTest<TVersion>(
				Failure_NetworkConfig_AccountVersion_Less_Than_Minimum,
				networkConfig,
				test::GetSupportedEntityVersionsString(),
				TType);
	}

	TRAITS_BASED_TEST(FailureWhenHarvestBeneficiaryPercentageInvalid) {
		auto networkConfig = networkConfigWithPlugin;
		boost::algorithm::replace_first(networkConfig, "harvestBeneficiaryPercentage = 10", "harvestBeneficiaryPercentage = 150");
		// Assert:
		RunTest<TVersion>(
			Failure_NetworkConfig_HarvestBeneficiaryPercentage_Exceeds_One_Hundred,
			networkConfig,
			test::GetSupportedEntityVersionsString(),
			TType);
	}

	TRAITS_BASED_TEST(FailureWhenMaxMosaicAtomicUnitsInvalid) {
		auto networkConfig = networkConfigWithPlugin;
		boost::algorithm::replace_first(networkConfig, "maxMosaicAtomicUnits = 9'000'000'000'000'000", "maxMosaicAtomicUnits = 10");
		// Assert:
		RunTest<TVersion>(
			Failure_NetworkConfig_MaxMosaicAtomicUnits_Invalid,
			networkConfig,
			test::GetSupportedEntityVersionsString(),
			TType);
	}

	TRAITS_BASED_TEST(FailureWhenBlockChainConfigInvalid) {
		// Assert:
		RunTest<TVersion>(
			Failure_NetworkConfig_BlockChain_Config_Malformed,
			"invalidConfig",
			test::GetSupportedEntityVersionsString(),
			TType);
	}

	TRAITS_BASED_TEST(FailureWhenPluginConfigMissing) {
		// Assert:
		RunTest<TVersion>(
			Failure_NetworkConfig_Plugin_Config_Missing,
			commonBlockChainProperties,
			test::GetSupportedEntityVersionsString(),
			TType);
	}

	TRAITS_BASED_TEST(FailureWhenPluginConfigInvalid) {
		// Assert:
		auto networkConfig = commonBlockChainProperties +
			"[plugin:catapult.plugins.config]\n\n"
			"qwerty = 12";
		RunTestWithRealPlugins<TVersion>(
			Failure_NetworkConfig_Plugin_Config_Malformed,
			networkConfig,
			test::GetSupportedEntityVersionsString(),
			TType);
	}

	TRAITS_BASED_TEST(FailureWhenNetworkConfigTransactionHasNoSupportedVersions) {
		// Assert:
		RunTest<TVersion>(
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
			"}",
			TType);
	}

	TRAITS_BASED_TEST(FailureWhenSupportedEntityVersionsConfigInvalid) {
		// Assert:
		RunTest<TVersion>(
			Failure_NetworkConfig_SupportedEntityVersions_Config_Malformed,
			networkConfigWithPlugin,
			"invalidConfig",
			TType);
	}

	TRAITS_BASED_TEST(Success) {
		// Assert:
		RunTestWithRealPlugins<TVersion>(
			ValidationResult::Success,
			networkConfigWithPlugin,
			test::GetSupportedEntityVersionsString(),
			TType);
	}
}}
