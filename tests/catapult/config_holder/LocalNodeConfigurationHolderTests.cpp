/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "plugins/txes/config/tests/test/NetworkConfigTestUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/other/MutableBlockchainConfiguration.h"
#include "tests/TestHarness.h"
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
		BlockchainConfigurationHolder testee(nullptr);

		// Act:
		auto& result = testee.LoadConfig(0, nullptr, "server");

		// Assert:
		EXPECT_EQ(13, result.Extensions.Names.size());
	}

	TEST(TEST_CLASS, CanLoadRecoveryConfig) {
		// Arrange:
		BlockchainConfigurationHolder testee(nullptr);

		// Act:
		auto& result = testee.LoadConfig(0, nullptr, "recovery");

		// Assert:
		EXPECT_EQ(1, result.Extensions.Names.size());
	}

	TEST(TEST_CLASS, CanLoadBrokerConfig) {
		// Arrange:
		BlockchainConfigurationHolder testee(nullptr);

		// Act:
		auto& result = testee.LoadConfig(0, nullptr, "broker");

		// Assert:
		EXPECT_EQ(4, result.Extensions.Names.size());
	}

	TEST(TEST_CLASS, LoadConfigThrowsWhenThereIsNoResourceFiles) {
		// Arrange:
		BlockchainConfigurationHolder testee(nullptr);
			const char* argv[] = { "", "/wrong_path" };

		// Act + Assert:
		EXPECT_THROW(testee.LoadConfig(2, argv, "server"), catapult_runtime_error);
	}

	// endregion

	// region SetConfig

	TEST(TEST_CLASS, CanSetConfig) {
		// Arrange:
		BlockchainConfigurationHolder testee(nullptr);
		test::MutableBlockchainConfiguration config;
		config.Network.ImportanceGrouping = 5;

		// Act:
		testee.SetConfig(Height{777}, config.ToConst());

		// Assert:
		EXPECT_EQ(5, testee.Config(Height{777}).Network.ImportanceGrouping);
	}

	TEST(TEST_CLASS, CanSetConfigMoreThanOnce) {
		// Arrange:
		BlockchainConfigurationHolder testee(nullptr);
		test::MutableBlockchainConfiguration config;
		config.Network.ImportanceGrouping = 5;
		testee.SetConfig(Height{777}, config.ToConst());
		EXPECT_EQ(5, testee.Config(Height{777}).Network.ImportanceGrouping);
		config.Network.ImportanceGrouping = 7;

		// Act:
		testee.SetConfig(Height{777}, config.ToConst());

		// Assert:
		EXPECT_EQ(7, testee.Config(Height{777}).Network.ImportanceGrouping);
	}

	// region Config(const Height& height)

	TEST(TEST_CLASS, GetDefaultConfigAtHeightOne) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache<test::NetworkConfigCacheFactory>();
		BlockchainConfigurationHolder testee(&cache);
		test::MutableBlockchainConfiguration config;
		config.Network.ImportanceGrouping = 5;
		testee.SetConfig(Height{0}, config.ToConst());

		// Act:
		auto& result = testee.Config(Height{777});

		// Assert:
		EXPECT_EQ(5, testee.Config(Height{777}).Network.ImportanceGrouping);
		EXPECT_EQ(0, result.SupportedEntityVersions.size());
	}

	TEST(TEST_CLASS, CanGetConfigAtHeight) {
		// Arrange:
		BlockchainConfigurationHolder testee(nullptr);
		test::MutableBlockchainConfiguration config;
		config.Network.ImportanceGrouping = 5;
		testee.SetConfig(Height{777}, config.ToConst());

		// Act:
		auto& result = testee.Config(Height{777});

		// Assert:
		EXPECT_EQ(5, testee.Config(Height{777}).Network.ImportanceGrouping);
		EXPECT_EQ(0, result.SupportedEntityVersions.size());
	}

	TEST(TEST_CLASS, CanGetConfigAtHeightFromCache) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache<test::NetworkConfigCacheFactory>();
		BlockchainConfigurationHolder testee(&cache);
		auto delta = cache.createDelta();
		auto& configCacheDelta = delta.sub<cache::NetworkConfigCache>();
		configCacheDelta.insert(state::NetworkConfigEntry(Height(555), networkConfig, test::GetSupportedEntityVersionsString()));
		cache.commit(Height(1));

		// Act:
		auto& result = testee.Config(Height{777});

		// Assert:
		EXPECT_EQ(7, result.Network.ImportanceGrouping);
		EXPECT_EQ(24, result.SupportedEntityVersions.size());
	}

	TEST(TEST_CLASS, GetConfigThrowsWhenCacheNotSet) {
		// Arrange:
		BlockchainConfigurationHolder testee(nullptr);

		// Act + Assert:
		EXPECT_THROW(testee.Config(Height{777}), catapult_invalid_argument);
	}

	namespace test_config_at_height {
		class TestBlockchainConfigurationHolder : public BlockchainConfigurationHolder {
		public:
			using BlockchainConfigurationHolder::BlockchainConfigurationHolder;

			void RemoveConfigAtZeroHeight() {
				m_networkConfigs.erase(Height{0});
			}
		};
	}

	TEST(TEST_CLASS, GetConfigThrowsWhenConfigIsNotFoundInCache) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache<test::NetworkConfigCacheFactory>();
		test_config_at_height::TestBlockchainConfigurationHolder testee(&cache);
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

			BlockchainConfiguration& Config(const Height& height) override {
				ConfigCalledAtHeight = height;
				return NetworkConfig;
			}

		public:
			Height ConfigCalledAtHeight;
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
		auto cache = test::CreateEmptyCatapultCache<test::NetworkConfigCacheFactory>();
		test_config::TestBlockchainConfigurationHolder testee(&cache);
		auto delta = cache.createDelta();
		auto& configCacheDelta = delta.sub<cache::NetworkConfigCache>();
		configCacheDelta.insert(state::NetworkConfigEntry(Height(555), "aaa", "bbb"));
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
			TestBlockchainConfigurationHolder(cache::CatapultCache* pCache)
				: BlockchainConfigurationHolder(pCache)
				, NetworkConfig(test::MutableBlockchainConfiguration().ToConst())
			{}

			BlockchainConfiguration& Config(const Height&) override {
				ConfigAtHeightCalled = true;
				return NetworkConfig;
			}

			BlockchainConfiguration& Config() override {
				ConfigCalled = true;
				return NetworkConfig;
			}

		public:
			bool ConfigAtHeightCalled = false;
			bool ConfigCalled = false;
			BlockchainConfiguration NetworkConfig;
		};
	}

	TEST(TEST_CLASS, ConfigCalledAtArbitraryHeight) {
		// Arrange:
		test_config_at_height_or_latest::TestBlockchainConfigurationHolder testee(nullptr);

		// Act:
		testee.ConfigAtHeightOrLatest(Height{777});

		// Assert:
		EXPECT_TRUE(testee.ConfigAtHeightCalled);
		EXPECT_FALSE(testee.ConfigCalled);
	}

	TEST(TEST_CLASS, ConfigCalledAtDefaultHeight) {
		// Arrange:
		test_config_at_height_or_latest::TestBlockchainConfigurationHolder testee(nullptr);

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
		test_set_cache::TestBlockchainConfigurationHolder testee(nullptr);

		// Assert:
		EXPECT_FALSE(testee.IsCacheSet());
	}

	TEST(TEST_CLASS, CacheSetAtConstruction) {
		// Act:
		auto cache = test::CreateEmptyCatapultCache<test::NetworkConfigCacheFactory>();
		test_set_cache::TestBlockchainConfigurationHolder testee(&cache);

		// Assert:
		EXPECT_TRUE(testee.IsCacheSet());
	}

	TEST(TEST_CLASS, CacheSetLater) {
		// Arrange:
		test_set_cache::TestBlockchainConfigurationHolder testee(nullptr);
		EXPECT_FALSE(testee.IsCacheSet());
		auto cache = test::CreateEmptyCatapultCache<test::NetworkConfigCacheFactory>();

		// Act:
		testee.SetCache(&cache);

		// Assert:
		EXPECT_TRUE(testee.IsCacheSet());
	}

	// endregion

	// region thread safety

	TEST(TEST_CLASS, ConfigHolderIsThreadSafe) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache<test::NetworkConfigCacheFactory>();
		BlockchainConfigurationHolder testee(&cache);
		uint64_t iterationCount = 1000;

		// Act:
		boost::thread_group threads;
		threads.create_thread([&testee, iterationCount] {
			test::MutableBlockchainConfiguration mutableConfig;
			for (uint64_t i = 1; i <= iterationCount; ++i) {
				mutableConfig.Network.ImportanceGrouping = i;
				testee.SetConfig(Height { i }, mutableConfig.ToConst());
			}
		});

		threads.create_thread([&testee, iterationCount] {
			for (;;) {
				auto& config = testee.Config(Height { iterationCount });
				if (config.Network.ImportanceGrouping == iterationCount)
					break;
			}
		});

		// - wait for all threads
		threads.join_all();
	}

	// endregion
}}
