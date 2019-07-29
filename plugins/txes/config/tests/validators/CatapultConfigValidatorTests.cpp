/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/CatapultConfigCache.h"
#include "src/cache/CatapultConfigCacheStorage.h"
#include "src/config/CatapultConfigConfiguration.h"
#include "src/validators/Validators.h"
#include "tests/test/CatapultConfigTestUtils.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/plugins/ValidatorTestUtils.h"
#include "tests/test/other/MutableCatapultConfiguration.h"
#include "tests/TestHarness.h"

namespace catapult { namespace validators {

#define TEST_CLASS CatapultConfigValidatorTests

	DEFINE_COMMON_VALIDATOR_TESTS(CatapultConfig, plugins::PluginManager(config::CreateMockConfigurationHolder(), plugins::StorageConfiguration()))

	namespace {
		std::string commonBlockChainProperties{
			"[network]\n"
			"\n"
			"identifier = mijin-test\n"
			"publicKey = B4F12E7C9F6946091E2CB8B6D3A12B50D17CCBBF646386EA27CE2946A7423DCF\n"
			"generationHash = 57F7DA205008026C776CB6AED843393F04CD458E0AA2D9F1D5F31A402072B2D6\n"
			"\n"
			"[chain]\n"
			"\n"
			"shouldEnableVerifiableState = true\n"
			"shouldEnableVerifiableReceipts = true\n"
			"\n"
			"currencyMosaicId = 0x0DC6'7FBE'1CAD'29E3\n"
			"harvestingMosaicId = 0x0DC6'7FBE'1CAD'29E3\n"
			"\n"
			"blockGenerationTargetTime = 15s\n"
			"blockTimeSmoothingFactor = 3000\n"
			"\n"
			"greedDelta = 0.5\n"
			"greedExponent = 2\n"
			"\n"
			"# maxTransactionLifetime / blockGenerationTargetTime\n"
			"importanceGrouping = 5760\n"
			"maxRollbackBlocks = 360\n"
			"maxDifficultyBlocks = 3\n"
			"\n"
			"maxTransactionLifetime = 24h\n"
			"maxBlockFutureTime = 10s\n"
			"\n"
			"initialCurrencyAtomicUnits = 8'999'999'998'000'000\n"
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

		std::shared_ptr<plugins::PluginManager> CreatePluginManager(uint64_t maxBlockChainConfigSizeMb, uint64_t maxSupportedEntityVersionsSizeMb) {
			test::MutableCatapultConfiguration config;
			auto pluginConfig = config::CatapultConfigConfiguration::Uninitialized();
			pluginConfig.MaxBlockChainConfigSize = utils::FileSize::FromMegabytes(maxBlockChainConfigSizeMb);
			pluginConfig.MaxSupportedEntityVersionsSize = utils::FileSize::FromMegabytes(maxSupportedEntityVersionsSizeMb);
			config.BlockChain.SetPluginConfiguration(PLUGIN_NAME(config), pluginConfig);
			auto pConfigHolder = config::CreateMockConfigurationHolder(config.ToConst());
			return std::make_shared<plugins::PluginManager>(pConfigHolder, plugins::StorageConfiguration());
		}

		std::shared_ptr<plugins::PluginManager> CreatePluginManagerWithRealPlugins(uint64_t maxBlockChainConfigSizeMb, uint64_t maxSupportedEntityVersionsSizeMb) {
			test::MutableCatapultConfiguration config;
			auto blockChainConfig = commonBlockChainProperties + pluginConfigs;
			std::istringstream input(blockChainConfig);
			config.BlockChain = model::BlockChainConfiguration::LoadFromBag(utils::ConfigurationBag::FromStream(input));
			auto pluginConfig = config::CatapultConfigConfiguration::Uninitialized();
			pluginConfig.MaxBlockChainConfigSize = utils::FileSize::FromMegabytes(maxBlockChainConfigSizeMb);
			pluginConfig.MaxSupportedEntityVersionsSize = utils::FileSize::FromMegabytes(maxSupportedEntityVersionsSizeMb);
			config.BlockChain.SetPluginConfiguration(PLUGIN_NAME(config), pluginConfig);
			return test::CreatePluginManagerWithRealPlugins(config.BlockChain);
		}

		void AssertValidationResult(
				ValidationResult expectedResult,
				const std::string& blockChainConfig,
				const std::string& supportedEntityVersions,
				std::shared_ptr<plugins::PluginManager> pPluginManager,
				cache::CatapultCache& cache) {
			// Arrange:
			model::CatapultConfigNotification<1> notification(
				BlockDuration(),
				blockChainConfig.size(),
				reinterpret_cast<const uint8_t*>(blockChainConfig.data()),
				supportedEntityVersions.size(),
				reinterpret_cast<const uint8_t*>(supportedEntityVersions.data()));
			auto pValidator = CreateCatapultConfigValidator(*pPluginManager);

			// Act:
			auto result = test::ValidateNotification(*pValidator, notification, cache, Height(1));

			// Assert:
			EXPECT_EQ(expectedResult, result);
		}

		void RunTest(
				ValidationResult expectedResult,
				const std::string& blockChainConfig,
				const std::string& supportedEntityVersions,
				uint64_t maxBlockChainConfigSizeMb = 1,
				uint64_t maxSupportedEntityVersionsSizeMb = 1,
				bool seedConfigCache = false) {
			auto pPluginManager = CreatePluginManager(maxBlockChainConfigSizeMb, maxSupportedEntityVersionsSizeMb);
			auto cache = test::CreateEmptyCatapultCache<test::CatapultConfigCacheFactory>(model::BlockChainConfiguration::Uninitialized());
			if (seedConfigCache) {
				auto delta = cache.createDelta();
				auto& congigCacheDelta = delta.sub<cache::CatapultConfigCache>();
				congigCacheDelta.insert(state::CatapultConfigEntry(Height(1), "BlockChainConfig", "SupportedEntityVersionsConfig"));
				cache.commit(Height(1));
			}
			AssertValidationResult(expectedResult, blockChainConfig, supportedEntityVersions, pPluginManager, cache);
		}

		void RunTestWithRealPlugins(
				ValidationResult expectedResult,
				const std::string& blockChainConfig,
				const std::string& supportedEntityVersions,
				uint64_t maxBlockChainConfigSizeMb = 1,
				uint64_t maxSupportedEntityVersionsSizeMb = 1) {
			auto pPluginManager = CreatePluginManagerWithRealPlugins(maxBlockChainConfigSizeMb, maxSupportedEntityVersionsSizeMb);
			auto cache = pPluginManager->createCache();
			AssertValidationResult(expectedResult, blockChainConfig, supportedEntityVersions, pPluginManager, cache);
		}
	}

	TEST(TEST_CLASS, FailureWhenBlockChainConfigTooBig) {
		// Assert:
		RunTest(
			Failure_CatapultConfig_BlockChain_Config_Too_Large,
			commonBlockChainProperties,
			test::GetSupportedEntityVersionsString(),
			0,
			1);
	}

	TEST(TEST_CLASS, FailureWhenSupportedEntityVersionsConfigTooBig) {
		// Assert:
		RunTest(
			Failure_CatapultConfig_SupportedEntityVersions_Config_Too_Large,
			commonBlockChainProperties,
			test::GetSupportedEntityVersionsString(),
			1,
			0);
	}

	TEST(TEST_CLASS, FailureCatapultConfigExistsAtHeight) {
		// Assert:
		RunTest(
			Failure_CatapultConfig_Config_Redundant,
			commonBlockChainProperties,
			test::GetSupportedEntityVersionsString(),
			1,
			1,
			true);
	}

	TEST(TEST_CLASS, FailureWhenBlockChainConfigInvalid) {
		// Assert:
		RunTest(
			Failure_CatapultConfig_BlockChain_Config_Malformed,
			"invalidConfig",
			test::GetSupportedEntityVersionsString());
	}

	TEST(TEST_CLASS, FailureWhenPluginConfigInvalid) {
		// Assert:
		auto blockChainConfig = commonBlockChainProperties +
			"[plugin:catapult.plugins.config]\n\n"
			"qwerty = 12";
		RunTestWithRealPlugins(
			Failure_CatapultConfig_Plugin_Config_Malformed,
			blockChainConfig,
			test::GetSupportedEntityVersionsString());
	}

	TEST(TEST_CLASS, FailureWhenCatapultConfigTransactionHasNoSupportedVersions) {
		// Assert:
		RunTest(
			Failure_CatapultConfig_Catapult_Config_Trx_Cannot_Be_Unsupported,
			commonBlockChainProperties,
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
			Failure_CatapultConfig_SupportedEntityVersions_Config_Malformed,
			commonBlockChainProperties,
			"invalidConfig");
	}

	TEST(TEST_CLASS, Success) {
		// Assert:
		auto blockChainConfig = commonBlockChainProperties + pluginConfigs;
		RunTestWithRealPlugins(
			ValidationResult::Success,
			blockChainConfig,
			test::GetSupportedEntityVersionsString());
	}
}}
