/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include <boost/thread.hpp>

namespace catapult { namespace config {

#define TEST_CLASS BlockchainConfigurationHolderTests

	namespace {
		std::string networkConfig{
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
			"importanceGrouping = 7\n"
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
			"[plugin:catapult.plugins.config]\n"
			"\n"
			"maxBlockChainConfigSize = 1MB\n"
			"maxSupportedEntityVersionsSize = 1MB\n"
		};
	}

	// region GetResourcesPath

	TEST(TEST_CLASS, GetResourcesPathWithoutArgs) {
		// Act:
		auto result = BlockchainConfigurationHolder::GetResourcesPath(0, nullptr);

		// Assert:
		EXPECT_EQ("../resources", result);
	}

	TEST(TEST_CLASS, GetResourcesPathWithArgs) {
		// Act:
		const char* argv[] = { "", "test" };
		auto result = BlockchainConfigurationHolder::GetResourcesPath(2, argv);

		// Assert:
		EXPECT_EQ("test/resources", result);
	}

	// endregion

	// region LoadConfig

	TEST(TEST_CLASS, CanLoadServerConfig) {
		// Arrange:
		BlockchainConfigurationHolder testee;

		// Act:
		auto resourcesPath = config::BlockchainConfigurationHolder::GetResourcesPath(0, nullptr);
		auto result = config::BlockchainConfiguration::LoadFromPath(resourcesPath, "server");

		// Assert:
		EXPECT_EQ(13, result.Extensions.Names.size());
	}

	TEST(TEST_CLASS, CanLoadRecoveryConfig) {
		// Arrange:
		BlockchainConfigurationHolder testee;

		// Act:
		auto resourcesPath = config::BlockchainConfigurationHolder::GetResourcesPath(0, nullptr);
		auto result = config::BlockchainConfiguration::LoadFromPath(resourcesPath, "recovery");

		// Assert:
		EXPECT_EQ(1, result.Extensions.Names.size());
	}

	TEST(TEST_CLASS, CanLoadBrokerConfig) {
		// Arrange:
		BlockchainConfigurationHolder testee;

		// Act:
		auto resourcesPath = config::BlockchainConfigurationHolder::GetResourcesPath(0, nullptr);
		auto result = config::BlockchainConfiguration::LoadFromPath(resourcesPath, "broker");

		// Assert:
		EXPECT_EQ(4, result.Extensions.Names.size());
	}

	TEST(TEST_CLASS, LoadConfigThrowsWhenThereIsNoResourceFiles) {
		// Arrange:
		BlockchainConfigurationHolder testee;
		const char* argv[] = { "", "/wrong_path" };
		auto resourcesPath = config::BlockchainConfigurationHolder::GetResourcesPath(2, argv);

		// Act + Assert:
		EXPECT_THROW(config::BlockchainConfiguration::LoadFromPath(resourcesPath, "server"), catapult_runtime_error);
	}

	// endregion

	// region Constructor with config

	TEST(TEST_CLASS, GetDefaultConfigAtHeightOne) {
		// Arrange:

		test::MutableBlockchainConfiguration config;
		config.Network.ImportanceGrouping = 5;
		BlockchainConfigurationHolder testee(config.ToConst());

		// Act:
		auto& result = testee.Config(Height{777});

		// Assert:
		EXPECT_EQ(5, testee.Config(Height{777}).Network.ImportanceGrouping);
		EXPECT_EQ(0, result.SupportedEntityVersions.size());
	}

	// end region

	namespace test_config_at_height {
		class TestBlockchainConfigurationHolder : public BlockchainConfigurationHolder {
		public:
			using BlockchainConfigurationHolder::BlockchainConfigurationHolder;

			void RemoveConfigAtZeroHeight() {
				m_configs.erase(Height{0});
			}
		};
	}

	TEST(TEST_CLASS, GetConfigThrowsWhenConfigIsNotFoundInCache) {
		// Arrange:
		test_config_at_height::TestBlockchainConfigurationHolder testee;
		testee.RemoveConfigAtZeroHeight();

		// Act + Assert:
		EXPECT_THROW(testee.Config(Height{777}), catapult_invalid_argument);
	}

	// endregion

	// region Config()

	namespace test_config {
		class TestBlockchainConfigurationHolder : public BlockchainConfigurationHolder {
		public:
			TestBlockchainConfigurationHolder(cache::CatapultCache* pCache)
				: BlockchainConfigurationHolder(pCache)
				, NetworkConfig(test::MutableBlockchainConfiguration().ToConst())
			{}

			const BlockchainConfiguration& Config(const Height& height) const override {
				ConfigCalledAtHeight = height;
				return NetworkConfig;
			}

		public:
			mutable Height ConfigCalledAtHeight;
			BlockchainConfiguration NetworkConfig;
		};
	}

	TEST(TEST_CLASS, ConfigCalledAtZeroHeightWhenCacheNotSet) {
		// Arrange:
		test_config::TestBlockchainConfigurationHolder testee(nullptr);

		// Act:
		testee.BlockchainConfigurationHolder::Config();

		// Assert:
		EXPECT_EQ(Height{0}, testee.ConfigCalledAtHeight);
	}

	TEST(TEST_CLASS, ConfigCalledAtCacheHeightWhenCacheSet) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache();
		test_config::TestBlockchainConfigurationHolder testee(&cache);

		auto delta = cache.createDelta();
		auto& accountCacheDelta = delta.sub<cache::AccountStateCache>();
		accountCacheDelta.addAccount(test::GenerateRandomByteArray<Key>(), Height(111));
		cache.commit(Height{777});

		// Act:
		testee.BlockchainConfigurationHolder::Config();

		// Assert:
		EXPECT_EQ(Height{777}, testee.ConfigCalledAtHeight);
	}

	// endregion

	// region ConfigAtHeightOrLatest

	namespace test_config_at_height_or_latest {
		class TestBlockchainConfigurationHolder : public BlockchainConfigurationHolder {
		public:
			TestBlockchainConfigurationHolder()
				: BlockchainConfigurationHolder()
				, NetworkConfig(test::MutableBlockchainConfiguration().ToConst())
			{}

			const BlockchainConfiguration& Config(const Height&) const override {
				ConfigAtHeightCalled = true;
				return NetworkConfig;
			}

			const BlockchainConfiguration& Config() const override {
				ConfigCalled = true;
				return NetworkConfig;
			}

		public:
			mutable bool ConfigAtHeightCalled = false;
			mutable bool ConfigCalled = false;
			BlockchainConfiguration NetworkConfig;
		};
	}

	TEST(TEST_CLASS, ConfigCalledAtArbitraryHeight) {
		// Arrange:
		test_config_at_height_or_latest::TestBlockchainConfigurationHolder testee;

		// Act:
		testee.ConfigAtHeightOrLatest(Height{777});

		// Assert:
		EXPECT_TRUE(testee.ConfigAtHeightCalled);
		EXPECT_FALSE(testee.ConfigCalled);
	}

	TEST(TEST_CLASS, ConfigCalledAtDefaultHeight) {
		// Arrange:
		test_config_at_height_or_latest::TestBlockchainConfigurationHolder testee;

		// Act:
		testee.ConfigAtHeightOrLatest(HEIGHT_OF_LATEST_CONFIG);

		// Assert:
		EXPECT_TRUE(testee.ConfigCalled);
		EXPECT_FALSE(testee.ConfigAtHeightCalled);
	}

	// endregion

	// region SetCache

	namespace test_set_cache {
		class TestBlockchainConfigurationHolder : public BlockchainConfigurationHolder {
		public:
			using BlockchainConfigurationHolder::BlockchainConfigurationHolder;

			bool IsCacheSet() {
				return !!m_pCache;
			}
		};
	}

	TEST(TEST_CLASS, CacheNotSet) {
		// Act:
		test_set_cache::TestBlockchainConfigurationHolder testee;

		// Assert:
		EXPECT_FALSE(testee.IsCacheSet());
	}

	TEST(TEST_CLASS, CacheSetAtConstruction) {
		// Act:
		auto cache = test::CreateEmptyCatapultCache();
		test_set_cache::TestBlockchainConfigurationHolder testee(&cache);

		// Assert:
		EXPECT_TRUE(testee.IsCacheSet());
	}

	TEST(TEST_CLASS, CacheSetLater) {
		// Arrange:
		test_set_cache::TestBlockchainConfigurationHolder testee;
		EXPECT_FALSE(testee.IsCacheSet());
		auto cache = test::CreateEmptyCatapultCache();

		// Act:
		testee.SetCache(&cache);

		// Assert:
		EXPECT_TRUE(testee.IsCacheSet());
	}

	// endregion

	// region thread safety
	// endregion

	// region total chain currency validation

	namespace {
		namespace inflation {
			class TestBlockchainConfigurationHolder : public BlockchainConfigurationHolder {
			public:
				TestBlockchainConfigurationHolder(const config::BlockchainConfiguration& config)
					: BlockchainConfigurationHolder(config)
				{}
				model::InflationCalculator& GetCalculator() {
					return *m_pInflationCalculator;
				}


			};
		}


		struct InflationEntry {
			catapult::Height Height;
			catapult::Amount Amount;
		};

		namespace {
			const char* Valid_Private_Key = "3485D98EFD7EB07ABAFCFD1A157D89DE2796A95E780813C0258AF3F5F84ED8CB";
			auto CreateMutableBlockchainConfiguration() {
				test::MutableBlockchainConfiguration config;
				config.Immutable.NetworkIdentifier = model::NetworkIdentifier::Mijin_Test;

				auto& networkConfig = config.Network;
				networkConfig.ImportanceGrouping = 1;
				networkConfig.MaxMosaicAtomicUnits = Amount(1000);
				networkConfig.MaxRollbackBlocks = 5u;
				networkConfig.ImportanceGrouping = 10u;
				networkConfig.HarvestBeneficiaryPercentage = 10;

				auto& nodeConfig = config.Node;
				nodeConfig.FeeInterest = 1;
				nodeConfig.FeeInterestDenominator = 1;

				auto& userConfig = config.User;
				userConfig.BootKey = Valid_Private_Key;

				return config;
			}

			auto CreateBlockchainConfigurationHolder(uint64_t initialCurrencyAtomicUnits, uint64_t maxMosaicAtomicUnits, uint64_t maxCurrencyAtomicUnits, const std::vector<InflationEntry>& inflationEntries) {
				auto mutableConfig = CreateMutableBlockchainConfiguration();

				mutableConfig.Immutable.InitialCurrencyAtomicUnits = Amount(initialCurrencyAtomicUnits);
				mutableConfig.Network.MaxMosaicAtomicUnits = Amount(maxMosaicAtomicUnits);
				mutableConfig.Network.MaxCurrencyMosaicAtomicUnits = Amount(maxCurrencyAtomicUnits);
				auto holder = std::make_shared<inflation::TestBlockchainConfigurationHolder>(mutableConfig.ToConst());

				model::InflationCalculator& calculator = holder->GetCalculator();
				for (const auto& inflationEntry : inflationEntries)
					calculator.add(inflationEntry.Height, inflationEntry.Amount, holder->Config().Network.MaxCurrencyMosaicAtomicUnits);

				// Sanity:
				EXPECT_EQ(inflationEntries.size(), calculator.size());

				return holder;
			}

			void ValidateConfiguration(std::shared_ptr<inflation::TestBlockchainConfigurationHolder> holder) {
				auto config = holder->Config();
				auto totalInflationUpToConfigActivation = holder->InflationCalculator().getCumulativeAmount(config.ActivationHeight);
				auto totalCurrency = holder->Config().Immutable.InitialCurrencyAtomicUnits + totalInflationUpToConfigActivation;
				/// totalCurrency at height of validation must not be higher than the new maxmosaicatomicunits
				if (config.Immutable.InitialCurrencyAtomicUnits > totalCurrency || totalCurrency > config.Network.MaxMosaicAtomicUnits)
					CATAPULT_THROW_AND_LOG_0(utils::property_malformed_error, "sum of InitialCurrencyAtomicUnits and inflation must not exceed MaxMosaicAtomicUnits");
				if(config.Network.MaxCurrencyMosaicAtomicUnits > config.Network.MaxMosaicAtomicUnits) {
					CATAPULT_THROW_AND_LOG_0(utils::property_malformed_error, "maxMosaicCurrencyAtomicUnits must not exceed maxMosaicAtomicUnits");;
				}
			}
		}
	}

	TEST(TEST_CLASS, SuccessfulValidationWhenConfigIsValid) {
		// Arrange: MaxMosaicAtomicUnits is 1000
		auto holder = CreateBlockchainConfigurationHolder(123, 1000, 1000, { { Height(1), Amount(1) }, { Height(234), Amount(0) } });

		// Act:
		EXPECT_NO_THROW(ValidateConfiguration(holder));
	}


	TEST(TEST_CLASS, ValidationFailWhenConfigInvalidDueToTotalCurrencyOverflows) {
		// Arrange:
		auto holder = CreateBlockchainConfigurationHolder(1005, 1000, 1000, { { Height(1), Amount(5) }, { Height(10), Amount(2) } });

		// Act:
		EXPECT_THROW(ValidateConfiguration(holder), utils::property_malformed_error);
	}

	TEST(TEST_CLASS, TerminalInflationMustBeZero) {
		// Arrange:
		auto holder = CreateBlockchainConfigurationHolder(100, 1000, 1200, { { Height(1), Amount(5) }, { Height(10), Amount(2) } });

		// Act:
		EXPECT_THROW(ValidateConfiguration(holder), utils::property_malformed_error);
	}

	// endregion
}}
