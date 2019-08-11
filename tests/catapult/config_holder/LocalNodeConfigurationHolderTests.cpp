/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/config_holder/LocalNodeConfigurationHolder.h"
#include "plugins/txes/config/tests/test/CatapultConfigTestUtils.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/other/MutableCatapultConfiguration.h"
#include "tests/TestHarness.h"
#include <boost/thread.hpp>

namespace catapult { namespace config {

#define TEST_CLASS LocalNodeConfigurationHolderTests

	namespace {
		std::string blockChainConfig{
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
			"importanceGrouping = 7\n"
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
			"[plugin:catapult.plugins.config]\n"
			"\n"
			"maxBlockChainConfigSize = 1MB\n"
			"maxSupportedEntityVersionsSize = 1MB\n"
		};
	}

	// region GetResourcesPath

	TEST(TEST_CLASS, GetResourcesPathWithoutArgs) {
		// Act:
		auto result = LocalNodeConfigurationHolder::GetResourcesPath(0, nullptr);

		// Assert:
		EXPECT_EQ("../resources", result);
	}

	TEST(TEST_CLASS, GetResourcesPathWithArgs) {
		// Act:
		const char* argv[] = { "", "test" };
		auto result = LocalNodeConfigurationHolder::GetResourcesPath(2, argv);

		// Assert:
		EXPECT_EQ("test/resources", result);
	}

	// endregion

	// region LoadConfig

	TEST(TEST_CLASS, CanLoadServerConfig) {
		// Arrange:
		LocalNodeConfigurationHolder testee(nullptr);

		// Act:
		auto& result = testee.LoadConfig(0, nullptr, "server");

		// Assert:
		EXPECT_EQ(13, result.Extensions.Names.size());
	}

	TEST(TEST_CLASS, CanLoadRecoveryConfig) {
		// Arrange:
		LocalNodeConfigurationHolder testee(nullptr);

		// Act:
		auto& result = testee.LoadConfig(0, nullptr, "recovery");

		// Assert:
		EXPECT_EQ(1, result.Extensions.Names.size());
	}

	TEST(TEST_CLASS, CanLoadBrokerConfig) {
		// Arrange:
		LocalNodeConfigurationHolder testee(nullptr);

		// Act:
		auto& result = testee.LoadConfig(0, nullptr, "broker");

		// Assert:
		EXPECT_EQ(4, result.Extensions.Names.size());
	}

	TEST(TEST_CLASS, LoadConfigThrowsWhenThereIsNoResourceFiles) {
		// Arrange:
		LocalNodeConfigurationHolder testee(nullptr);
			const char* argv[] = { "", "/wrong_path" };

		// Act + Assert:
		EXPECT_THROW(testee.LoadConfig(2, argv, "server"), catapult_runtime_error);
	}

	// endregion

	// region SetConfig

	TEST(TEST_CLASS, CanSetConfig) {
		// Arrange:
		LocalNodeConfigurationHolder testee(nullptr);
		test::MutableCatapultConfiguration config;
		config.BlockChain.ImportanceGrouping = 5;

		// Act:
		testee.SetConfig(Height{777}, config.ToConst());

		// Assert:
		EXPECT_EQ(5, testee.Config(Height{777}).BlockChain.ImportanceGrouping);
	}

	TEST(TEST_CLASS, CanSetConfigMoreThanOnce) {
		// Arrange:
		LocalNodeConfigurationHolder testee(nullptr);
		test::MutableCatapultConfiguration config;
		config.BlockChain.ImportanceGrouping = 5;
		testee.SetConfig(Height{777}, config.ToConst());
		EXPECT_EQ(5, testee.Config(Height{777}).BlockChain.ImportanceGrouping);
		config.BlockChain.ImportanceGrouping = 7;

		// Act:
		testee.SetConfig(Height{777}, config.ToConst());

		// Assert:
		EXPECT_EQ(7, testee.Config(Height{777}).BlockChain.ImportanceGrouping);
	}

	// region Config(const Height& height)

	TEST(TEST_CLASS, GetDefaultConfigAtHeightOne) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache<test::CatapultConfigCacheFactory>(model::BlockChainConfiguration::Uninitialized());
		LocalNodeConfigurationHolder testee(&cache);
		test::MutableCatapultConfiguration config;
		config.BlockChain.ImportanceGrouping = 5;
		testee.SetConfig(Height{0}, config.ToConst());

		// Act:
		auto& result = testee.Config(Height{777});

		// Assert:
		EXPECT_EQ(5, testee.Config(Height{777}).BlockChain.ImportanceGrouping);
		EXPECT_EQ(0, result.SupportedEntityVersions.size());
	}

	TEST(TEST_CLASS, CanGetConfigAtHeight) {
		// Arrange:
		LocalNodeConfigurationHolder testee(nullptr);
		test::MutableCatapultConfiguration config;
		config.BlockChain.ImportanceGrouping = 5;
		testee.SetConfig(Height{777}, config.ToConst());

		// Act:
		auto& result = testee.Config(Height{777});

		// Assert:
		EXPECT_EQ(5, testee.Config(Height{777}).BlockChain.ImportanceGrouping);
		EXPECT_EQ(0, result.SupportedEntityVersions.size());
	}

	TEST(TEST_CLASS, CanGetConfigAtHeightFromCache) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache<test::CatapultConfigCacheFactory>(model::BlockChainConfiguration::Uninitialized());
		LocalNodeConfigurationHolder testee(&cache);
		auto delta = cache.createDelta();
		auto& configCacheDelta = delta.sub<cache::CatapultConfigCache>();
		configCacheDelta.insert(state::CatapultConfigEntry(Height(555), blockChainConfig, test::GetSupportedEntityVersionsString()));
		cache.commit(Height(1));

		// Act:
		auto& result = testee.Config(Height{777});

		// Assert:
		EXPECT_EQ(7, result.BlockChain.ImportanceGrouping);
		EXPECT_EQ(24, result.SupportedEntityVersions.size());
	}

	TEST(TEST_CLASS, GetConfigThrowsWhenCacheNotSet) {
		// Arrange:
		LocalNodeConfigurationHolder testee(nullptr);

		// Act + Assert:
		EXPECT_THROW(testee.Config(Height{777}), catapult_invalid_argument);
	}

	namespace test_config_at_height {
		class TestLocalNodeConfigurationHolder : public LocalNodeConfigurationHolder {
		public:
			using LocalNodeConfigurationHolder::LocalNodeConfigurationHolder;

			void RemoveConfigAtZeroHeight() {
				m_catapultConfigs.erase(Height{0});
			}
		};
	}

	TEST(TEST_CLASS, GetConfigThrowsWhenConfigIsNotFoundInCache) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache<test::CatapultConfigCacheFactory>(model::BlockChainConfiguration::Uninitialized());
		test_config_at_height::TestLocalNodeConfigurationHolder testee(&cache);
		testee.RemoveConfigAtZeroHeight();

		// Act + Assert:
		EXPECT_THROW(testee.Config(Height{777}), catapult_invalid_argument);
	}

	// endregion

	// region Config()

	namespace test_config {
		class TestLocalNodeConfigurationHolder : public LocalNodeConfigurationHolder {
		public:
			TestLocalNodeConfigurationHolder(cache::CatapultCache* pCache)
				: LocalNodeConfigurationHolder(pCache)
				, CatapultConfig(test::MutableCatapultConfiguration().ToConst())
			{}

			CatapultConfiguration& Config(const Height& height) override {
				ConfigCalledAtHeight = height;
				return CatapultConfig;
			}

		public:
			Height ConfigCalledAtHeight;
			CatapultConfiguration CatapultConfig;
		};
	}

	TEST(TEST_CLASS, ConfigCalledAtZeroHeightWhenCacheNotSet) {
		// Arrange:
		test_config::TestLocalNodeConfigurationHolder testee(nullptr);

		// Act:
		testee.LocalNodeConfigurationHolder::Config();

		// Assert:
		EXPECT_EQ(Height{0}, testee.ConfigCalledAtHeight);
	}

	TEST(TEST_CLASS, ConfigCalledAtCacheHeightWhenCacheSet) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache<test::CatapultConfigCacheFactory>(model::BlockChainConfiguration::Uninitialized());
		test_config::TestLocalNodeConfigurationHolder testee(&cache);
		auto delta = cache.createDelta();
		auto& configCacheDelta = delta.sub<cache::CatapultConfigCache>();
		configCacheDelta.insert(state::CatapultConfigEntry(Height(555), "aaa", "bbb"));
		cache.commit(Height{777});

		// Act:
		testee.LocalNodeConfigurationHolder::Config();

		// Assert:
		EXPECT_EQ(Height{777}, testee.ConfigCalledAtHeight);
	}

	// endregion

	// region ConfigAtHeightOrLatest

	namespace test_config_at_height_or_latest {
		class TestLocalNodeConfigurationHolder : public LocalNodeConfigurationHolder {
		public:
			TestLocalNodeConfigurationHolder(cache::CatapultCache* pCache)
				: LocalNodeConfigurationHolder(pCache)
				, CatapultConfig(test::MutableCatapultConfiguration().ToConst())
			{}

			CatapultConfiguration& Config(const Height&) override {
				ConfigAtHeightCalled = true;
				return CatapultConfig;
			}

			CatapultConfiguration& Config() override {
				ConfigCalled = true;
				return CatapultConfig;
			}

		public:
			bool ConfigAtHeightCalled = false;
			bool ConfigCalled = false;
			CatapultConfiguration CatapultConfig;
		};
	}

	TEST(TEST_CLASS, ConfigCalledAtArbitraryHeight) {
		// Arrange:
		test_config_at_height_or_latest::TestLocalNodeConfigurationHolder testee(nullptr);

		// Act:
		testee.ConfigAtHeightOrLatest(Height{777});

		// Assert:
		EXPECT_TRUE(testee.ConfigAtHeightCalled);
		EXPECT_FALSE(testee.ConfigCalled);
	}

	TEST(TEST_CLASS, ConfigCalledAtDefaultHeight) {
		// Arrange:
		test_config_at_height_or_latest::TestLocalNodeConfigurationHolder testee(nullptr);

		// Act:
		testee.ConfigAtHeightOrLatest(HEIGHT_OF_LATEST_CONFIG);

		// Assert:
		EXPECT_TRUE(testee.ConfigCalled);
		EXPECT_FALSE(testee.ConfigAtHeightCalled);
	}

	// endregion

	// region SetCache

	namespace test_set_cache {
		class TestLocalNodeConfigurationHolder : public LocalNodeConfigurationHolder {
		public:
			using LocalNodeConfigurationHolder::LocalNodeConfigurationHolder;

			bool IsCacheSet() {
				return !!m_pCache;
			}
		};
	}

	TEST(TEST_CLASS, CacheNotSet) {
		// Act:
		test_set_cache::TestLocalNodeConfigurationHolder testee(nullptr);

		// Assert:
		EXPECT_FALSE(testee.IsCacheSet());
	}

	TEST(TEST_CLASS, CacheSetAtConstruction) {
		// Act:
		auto cache = test::CreateEmptyCatapultCache<test::CatapultConfigCacheFactory>(model::BlockChainConfiguration::Uninitialized());
		test_set_cache::TestLocalNodeConfigurationHolder testee(&cache);

		// Assert:
		EXPECT_TRUE(testee.IsCacheSet());
	}

	TEST(TEST_CLASS, CacheSetLater) {
		// Arrange:
		test_set_cache::TestLocalNodeConfigurationHolder testee(nullptr);
		EXPECT_FALSE(testee.IsCacheSet());
		auto cache = test::CreateEmptyCatapultCache<test::CatapultConfigCacheFactory>(model::BlockChainConfiguration::Uninitialized());

		// Act:
		testee.SetCache(&cache);

		// Assert:
		EXPECT_TRUE(testee.IsCacheSet());
	}

	// endregion

	// region thread safety

	TEST(TEST_CLASS, ConfigHolderIsThreadSafe) {
		// Arrange:
		auto cache = test::CreateEmptyCatapultCache<test::CatapultConfigCacheFactory>(model::BlockChainConfiguration::Uninitialized());
		LocalNodeConfigurationHolder testee(&cache);
		uint64_t iterationCount = 1000;
		test::MutableCatapultConfiguration mutableConfig;

		// Act:
		boost::thread_group threads;
		threads.create_thread([&testee, iterationCount, &mutableConfig] {
			for (uint64_t i = 0; i < iterationCount; ++i) {
				mutableConfig.BlockChain.ImportanceGrouping = i;
				testee.SetConfig(Height{777}, mutableConfig.ToConst());
			}
		});

		threads.create_thread([&testee, iterationCount] {
			for (;;) {
				auto& config = testee.Config(Height{777});
				if (config.BlockChain.ImportanceGrouping == (iterationCount - 1))
					break;
			}
		});

		// - wait for all threads
		threads.join_all();
	}

	// endregion
}}
