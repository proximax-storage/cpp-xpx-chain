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

#include "catapult/extensions/LocalNodeStateFileStorage.h"
#include "catapult/cache/CatapultCache.h"
#include "catapult/cache/SupplementalData.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/AccountStateCacheSubCachePlugin.h"
#include "catapult/cache_core/BlockDifficultyCache.h"
#include "catapult/cache_core/BlockDifficultyCacheSubCachePlugin.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "catapult/consumers/BlockChainSyncHandlers.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/io/IndexFile.h"
#include "catapult/model/Address.h"
#include "catapult/model/BlockChainConfiguration.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/AccountStateTestUtils.h"
#include "tests/test/core/mocks/MockLocalNodeConfigurationHolder.h"
#include "tests/test/local/LocalNodeTestState.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/nemesis/NemesisCompatibleConfiguration.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/test/other/MutableCatapultConfiguration.h"
#include "tests/test/plugins/PluginManagerFactory.h"
#include "tests/TestHarness.h"

namespace catapult { namespace extensions {

#define TEST_CLASS LocalNodeStateFileStorageTests

	namespace {
		constexpr auto Default_Network_Id = model::NetworkIdentifier::Mijin_Test;
		constexpr auto Harvesting_Mosaic_Id = MosaicId(9876);

		constexpr size_t Account_Cache_Size = 123;
		constexpr size_t Block_Cache_Size = 200;

		// region seed utils

		void PopulateAccountStateCache(cache::AccountStateCacheDelta& cacheDelta) {
			for (auto i = 2u; i < Account_Cache_Size + 2; ++i) {
				auto publicKey = test::GenerateRandomByteArray<Key>();
				state::AccountState* pAccountState;
				if (0 == i % 2) {
					cacheDelta.addAccount(publicKey, Height(i / 2));
					pAccountState = &cacheDelta.find(publicKey).get();
				} else {
					auto address = model::PublicKeyToAddress(publicKey, Default_Network_Id);
					cacheDelta.addAccount(address, Height(i / 2));
					pAccountState = &cacheDelta.find(address).get();
				}

				pAccountState->Balances.credit(Harvesting_Mosaic_Id, Amount(10));
				test::RandomFillAccountData(i, *pAccountState);
			}
		}

		void PopulateBlockDifficultyCache(cache::BlockDifficultyCacheDelta& cacheDelta) {
			for (auto i = 0u; i < Block_Cache_Size; ++i)
				cacheDelta.insert(Height(i), Timestamp(2 * i + 1), Difficulty(3 * i + 1));
		}

		void RandomSeedCache(cache::CatapultCache& catapultCache) {
			// Arrange: seed the cache with random data
			{
				auto delta = catapultCache.createDelta();
				PopulateAccountStateCache(delta.sub<cache::AccountStateCache>());
				PopulateBlockDifficultyCache(delta.sub<cache::BlockDifficultyCache>());
				catapultCache.commit(Height(54321));
			}

			// Sanity: data was seeded
			auto view = catapultCache.createView();
			EXPECT_EQ(Account_Cache_Size, view.sub<cache::AccountStateCache>().size());
			EXPECT_EQ(Block_Cache_Size, view.sub<cache::BlockDifficultyCache>().size());
		}

		cache::SupplementalData CreateDeterministicSupplementalData() {
			cache::SupplementalData supplementalData;
			supplementalData.ChainScore = model::ChainScore(0x1234567890ABCDEF, 0xFEDCBA0987654321);
			supplementalData.State.LastRecalculationHeight = model::ImportanceHeight(12345);
			supplementalData.State.NumTotalTransactions = 7654321;
			return supplementalData;
		}

		void PrepareAndSaveCompleteState(const config::CatapultDirectory& directory, cache::CatapultCache& cache) {
			// Arrange:
			auto supplementalData = CreateDeterministicSupplementalData();
			RandomSeedCache(cache);

			LocalNodeStateSerializer serializer(directory);
			serializer.save(cache, supplementalData.State, supplementalData.ChainScore);
		}

		void PrepareAndSaveSummaryState(const config::CatapultDirectory& directory, cache::CatapultCache& cache) {
			// Arrange:
			auto supplementalData = CreateDeterministicSupplementalData();
			RandomSeedCache(cache);

			auto storages = const_cast<const cache::CatapultCache&>(cache).storages();
			LocalNodeStateSerializer serializer(directory);
			serializer.save(cache.createDelta(), storages, supplementalData.State, supplementalData.ChainScore, Height(54321));
		}

		void AssertPreparedData(const StateHeights& heights, const LocalNodeStateRef& stateRef) {
			// Assert: check heights
			EXPECT_EQ(Height(54321), heights.Cache);
			EXPECT_EQ(Height(1), heights.Storage);

			// - check supplemental data
			EXPECT_EQ(model::ChainScore(0x1234567890ABCDEF, 0xFEDCBA0987654321), stateRef.Score.get());
			EXPECT_EQ(model::ImportanceHeight(12345), stateRef.State.LastRecalculationHeight);
			EXPECT_EQ(7654321u, stateRef.State.NumTotalTransactions);
			EXPECT_EQ(Height(54321), stateRef.Cache.createView().height());
		}

		// endregion
	}

	// region HasSerializedState

	TEST(TEST_CLASS, HasSerializedStateReturnsTrueWhenSupplementalDataFileExists) {
		// Arrange: create a sentinel file
		test::TempDirectoryGuard tempDir;
		auto stateDirectory = config::CatapultDirectory(tempDir.name() + "/zstate");
		boost::filesystem::create_directories(stateDirectory.path());
		io::IndexFile(stateDirectory.file("supplemental.dat")).set(123);

		// Act:
		auto result = HasSerializedState(stateDirectory);

		// Assert:
		EXPECT_TRUE(result);
	}

	TEST(TEST_CLASS, HasSerializedStateReturnsFalseWhenSupplementalDataFileDoesNotExist) {
		// Arrange:
		test::TempDirectoryGuard tempDir;
		auto stateDirectory = config::CatapultDirectory(tempDir.name() + "/zstate");

		// Act:
		auto result = HasSerializedState(stateDirectory);

		// Assert:
		EXPECT_FALSE(result);
	}

	// endregion

	// region LoadStateFromDirectory - first boot

	TEST(TEST_CLASS, NemesisBlockIsExecutedWhenSupplementalDataFileIsNotPresent) {
		// Arrange: seed and save the cache state (real plugin manager is needed to execute nemesis)
		test::TempDirectoryGuard tempDir;
		auto stateDirectory = config::CatapultDirectory(tempDir.name() + "/zstate");
		auto blockChainConfig = test::CreateCatapultConfigurationWithNemesisPluginExtensions("").BlockChain;

		{
			auto pPluginManager = test::CreatePluginManagerWithRealPlugins(blockChainConfig);
			auto originalCache = pPluginManager->createCache();
			PrepareAndSaveCompleteState(stateDirectory, originalCache);
		}

		// - remove the supplemental data file
		ASSERT_TRUE(boost::filesystem::remove(stateDirectory.file("supplemental.dat")));

		// Act: load the state
		auto pPluginManager = test::CreatePluginManagerWithRealPlugins(blockChainConfig);
		test::LocalNodeTestState loadedState(blockChainConfig, stateDirectory.str(), pPluginManager->createCache());
		auto heights = LoadStateFromDirectory(stateDirectory, loadedState.ref(), *pPluginManager);

		// Assert:
		EXPECT_EQ(Height(1), heights.Cache);
		EXPECT_EQ(Height(1), heights.Storage);

		EXPECT_EQ(model::ChainScore(1), loadedState.ref().Score.get());
		EXPECT_EQ(model::ImportanceHeight(), loadedState.ref().State.LastRecalculationHeight);
		EXPECT_EQ(Height(1), loadedState.ref().Cache.createView().height());
	}

	// endregion

	// region prepare helpers

	namespace {
		void PrepareNonexistentDirectory(const config::CatapultDirectory& directory) {
			// Sanity:
			EXPECT_FALSE(boost::filesystem::exists(directory.path()));
		}

		void PrepareEmptyDirectory(const config::CatapultDirectory& directory) {
			// Arrange:
			boost::filesystem::create_directories(directory.path());

			// Sanity:
			EXPECT_TRUE(boost::filesystem::exists(directory.path()));
		}

		void PrepareNonEmptyDirectory(const config::CatapultDirectory& directory) {
			// Arrange:
			boost::filesystem::create_directories(directory.path());
			io::IndexFile(directory.file("sentinel")).set(1);

			// Sanity:
			EXPECT_TRUE(boost::filesystem::exists(directory.path()));
			EXPECT_TRUE(boost::filesystem::exists(directory.file("sentinel")));
		}
	}

	// endregion

	// region LoadStateFromDirectory / LocalNodeStateSerializer (CatapultCache)

	namespace {
		template<typename TPrepare>
		void RunSaveAndLoadCompleteStateTest(TPrepare prepare) {
			// Arrange: seed and save the cache state with rocks disabled
			test::TempDirectoryGuard tempDir;
			auto stateDirectory = config::CatapultDirectory(tempDir.name() + "/zstate");
			auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
			blockChainConfig.HarvestingMosaicId = test::Default_Harvesting_Mosaic_Id;
			auto originalCache = test::CoreSystemCacheFactory::Create(blockChainConfig);

			// - run additional preparation
			prepare(stateDirectory);

			// Act: save the state
			PrepareAndSaveCompleteState(stateDirectory, originalCache);

			// Act: load the state
			test::LocalNodeTestState loadedState(
					blockChainConfig,
					stateDirectory.str(),
					test::CoreSystemCacheFactory::Create(blockChainConfig));
			auto pluginManager = test::CreatePluginManager(blockChainConfig);
			auto heights = LoadStateFromDirectory(stateDirectory, loadedState.ref(), pluginManager);

			// Assert:
			AssertPreparedData(heights, loadedState.ref());

			auto expectedView = originalCache.createView();
			auto actualView = loadedState.ref().Cache.createView();
			EXPECT_EQ(expectedView.sub<cache::AccountStateCache>().size(), actualView.sub<cache::AccountStateCache>().size());
			EXPECT_EQ(expectedView.sub<cache::BlockDifficultyCache>().size(), actualView.sub<cache::BlockDifficultyCache>().size());

			EXPECT_EQ(3u, test::CountFilesAndDirectories(stateDirectory.path()));
			for (const auto* supplementalFilename : { "supplemental.dat", "AccountStateCache.dat", "BlockDifficultyCache.dat" })
				EXPECT_TRUE(boost::filesystem::exists(stateDirectory.file(supplementalFilename))) << supplementalFilename;
		}
	}

	TEST(TEST_CLASS, CanSaveAndLoadCompleteState_DirectoryDoesNotExist) {
		// Act + Assert:
		RunSaveAndLoadCompleteStateTest(PrepareNonexistentDirectory);
	}

	TEST(TEST_CLASS, CanSaveAndLoadCompleteState_DirectoryExists) {
		// Act + Assert:
		RunSaveAndLoadCompleteStateTest(PrepareEmptyDirectory);
	}

	// endregion

	// region LoadStateFromDirectory / LocalNodeStateSerializer (CatapultCacheDelta)

	namespace {
		auto CreateConfigHolder() {
			test::MutableCatapultConfiguration config;

			config.BlockChain.MinHarvesterBalance = Amount(1);
			config.BlockChain.ImportanceGrouping = 1;
			config.BlockChain.HarvestingMosaicId = Harvesting_Mosaic_Id;
			config.BlockChain.MaxDifficultyBlocks = 111;

			auto pConfigHolder = std::make_shared<config::MockLocalNodeConfigurationHolder>();
			pConfigHolder->SetConfig(Height{0}, config.ToConst());
			return pConfigHolder;
		}

		cache::CatapultCache CreateCacheWithRealCoreSystemPlugins(const std::string& databaseDirectory) {
			auto cacheConfig = cache::CacheConfiguration(databaseDirectory, utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled);

			auto pConfigHolder = CreateConfigHolder();

			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches;
			subCaches.push_back(std::make_unique<cache::AccountStateCacheSubCachePlugin>(cacheConfig, pConfigHolder));
			subCaches.push_back(std::make_unique<cache::BlockDifficultyCacheSubCachePlugin>(pConfigHolder));
			return cache::CatapultCache(std::move(subCaches));
		}

		template<typename TPrepare>
		void RunSaveAndLoadSummaryStateTest(TPrepare prepare) {
			// Arrange: seed and save the cache state with rocks enabled
			test::TempDirectoryGuard tempDir;
			auto stateDirectory = config::CatapultDirectory(tempDir.name() + "/zstate");
			auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
			auto originalCache = CreateCacheWithRealCoreSystemPlugins(tempDir.name() + "/db");

			// - run additional preparation
			prepare(stateDirectory);

			// Act: save the state
			PrepareAndSaveSummaryState(stateDirectory, originalCache);

			// Act: load the state
			test::LocalNodeTestState loadedState(
					blockChainConfig,
					stateDirectory.str(),
					CreateCacheWithRealCoreSystemPlugins(tempDir.name() + "/db2"));
			auto pluginManager = test::CreatePluginManager();
			auto heights = LoadStateFromDirectory(stateDirectory, loadedState.ref(), pluginManager);

			// Assert:
			AssertPreparedData(heights, loadedState.ref());

			auto expectedView = originalCache.createView();
			auto actualView = loadedState.ref().Cache.createView();
			const auto& actualAccountStateCache = actualView.sub<cache::AccountStateCache>();
			EXPECT_EQ(0u, actualAccountStateCache.size());
			EXPECT_FALSE(actualAccountStateCache.highValueAddresses().empty());
			EXPECT_EQ(expectedView.sub<cache::AccountStateCache>().highValueAddresses(), actualAccountStateCache.highValueAddresses());
			EXPECT_EQ(expectedView.sub<cache::BlockDifficultyCache>().size(), actualView.sub<cache::BlockDifficultyCache>().size());

			EXPECT_EQ(3u, test::CountFilesAndDirectories(stateDirectory.path()));
			for (const auto* supplementalFilename : { "supplemental.dat", "AccountStateCache_summary.dat", "BlockDifficultyCache.dat" })
				EXPECT_TRUE(boost::filesystem::exists(stateDirectory.file(supplementalFilename))) << supplementalFilename;
		}
	}

	TEST(TEST_CLASS, CanSaveAndLoadSummaryState_DirectoryDoesNotExist) {
		// Act + Assert:
		RunSaveAndLoadSummaryStateTest(PrepareNonexistentDirectory);
	}

	TEST(TEST_CLASS, CanSaveAndLoadSummaryState_DirectoryExists) {
		// Act + Assert:
		RunSaveAndLoadSummaryStateTest(PrepareEmptyDirectory);
	}

	// endregion

	// region LocalNodeStateSerializer::moveTo

	namespace {
		template<typename TPrepare>
		void RunMoveToTest(TPrepare prepare) {
			// Arrange: write a file in the source directory
			test::TempDirectoryGuard tempDir;
			auto stateDirectory = config::CatapultDirectory(tempDir.name() + "/zstate");
			boost::filesystem::create_directories(stateDirectory.path());
			io::IndexFile(stateDirectory.file("sentinel")).set(123);

			// - run additional preparation on destination directory
			auto stateDirectory2 = config::CatapultDirectory(tempDir.name() + "/zstate2");
			prepare(stateDirectory2);

			// Act:
			LocalNodeStateSerializer serializer(stateDirectory);
			serializer.moveTo(stateDirectory2);

			// Assert:
			EXPECT_FALSE(boost::filesystem::exists(stateDirectory.file("sentinel")));
			EXPECT_TRUE(boost::filesystem::exists(stateDirectory2.file("sentinel")));

			EXPECT_EQ(123u, io::IndexFile(stateDirectory2.file("sentinel")).get());
		}
	}

	TEST(TEST_CLASS, CanMoveTo_DirectoryDoesNotExist) {
		// Act + Assert:
		RunMoveToTest(PrepareNonexistentDirectory);
	}

	TEST(TEST_CLASS, CanMoveTo_EmptyDirectoryExists) {
		// Act + Assert:
		RunMoveToTest(PrepareEmptyDirectory);
	}

	TEST(TEST_CLASS, CanMoveTo_NonEmptyDirectoryExists) {
		// Act + Assert:
		RunMoveToTest(PrepareNonEmptyDirectory);
	}

	// endregion

	// region SaveStateToDirectoryWithCheckpointing

	namespace {
		auto ReadCommitStep(const config::CatapultDataDirectory& dataDirectory) {
			return static_cast<consumers::CommitOperationStep>(io::IndexFile(dataDirectory.rootDir().file("commit_step.dat")).get());
		}
	}

	TEST(TEST_CLASS, SaveStateToDirectoryWithCheckpointing_CommitStepIsBlocksWrittenWhenSaveFails) {
		// Arrange: indicate rocks is enabled
		test::TempDirectoryGuard tempDir;
		auto dataDirectory = config::CatapultDataDirectory(tempDir.name());
		auto nodeConfig = config::NodeConfiguration::Uninitialized();
		nodeConfig.ShouldUseCacheDatabaseStorage = true;

		// - seed the cache state with rocks disabled
		auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
		auto catapultCache = test::CoreSystemCacheFactory::Create(blockChainConfig);
		RandomSeedCache(catapultCache);

		auto supplementalData = CreateDeterministicSupplementalData();

		// Act: save the state
		constexpr auto SaveState = SaveStateToDirectoryWithCheckpointing;
		EXPECT_THROW(
				SaveState(dataDirectory, nodeConfig, catapultCache, supplementalData.State, supplementalData.ChainScore),
				catapult_invalid_argument);

		// Assert:
		EXPECT_EQ(consumers::CommitOperationStep::Blocks_Written, ReadCommitStep(dataDirectory));
	}

	TEST(TEST_CLASS, SaveStateToDirectoryWithCheckpointing_CommitStepIsStateWrittenWhenMoveToFails) {
		// Arrange:
		test::TempDirectoryGuard tempDir;
		auto dataDirectory = config::CatapultDataDirectory(tempDir.name());
		auto nodeConfig = config::NodeConfiguration::Uninitialized();
		nodeConfig.ShouldUseCacheDatabaseStorage = false;

		// - seed the cache state with rocks disabled
		auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
		auto catapultCache = test::CoreSystemCacheFactory::Create(blockChainConfig);
		RandomSeedCache(catapultCache);

		auto supplementalData = CreateDeterministicSupplementalData();

		// - trigger a moveTo failure
		io::IndexFile(dataDirectory.rootDir().file("state")).set(0);

		// Act: save the state
		constexpr auto SaveState = SaveStateToDirectoryWithCheckpointing;
		EXPECT_THROW(
				SaveState(dataDirectory, nodeConfig, catapultCache, supplementalData.State, supplementalData.ChainScore),
				boost::filesystem::filesystem_error);

		// Assert:
		EXPECT_EQ(consumers::CommitOperationStep::State_Written, ReadCommitStep(dataDirectory));
	}

	TEST(TEST_CLASS, SaveStateToDirectoryWithCheckpointing_CommitStepIsAllUpdatedWhenCatapultCacheSaveSucceeds) {
		// Arrange:
		test::TempDirectoryGuard tempDir;
		auto dataDirectory = config::CatapultDataDirectory(tempDir.name());
		auto nodeConfig = config::NodeConfiguration::Uninitialized();
		nodeConfig.ShouldUseCacheDatabaseStorage = false;

		// - seed the cache state with rocks disabled
		auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
		auto catapultCache = test::CoreSystemCacheFactory::Create(blockChainConfig);
		RandomSeedCache(catapultCache);

		auto supplementalData = CreateDeterministicSupplementalData();

		// Act: save the state
		constexpr auto SaveState = SaveStateToDirectoryWithCheckpointing;
		SaveState(dataDirectory, nodeConfig, catapultCache, supplementalData.State, supplementalData.ChainScore);

		// Assert:
		EXPECT_EQ(consumers::CommitOperationStep::All_Updated, ReadCommitStep(dataDirectory));
	}

	TEST(TEST_CLASS, SaveStateToDirectoryWithCheckpointing_CommitStepIsAllUpdatedWhenCatapultCacheDeltaSaveSucceeds) {
		// Arrange:
		test::TempDirectoryGuard tempDir;
		auto dataDirectory = config::CatapultDataDirectory(tempDir.name());
		auto nodeConfig = config::NodeConfiguration::Uninitialized();
		nodeConfig.ShouldUseCacheDatabaseStorage = true;

		// - seed the cache state with rocks enabled
		auto catapultCache = CreateCacheWithRealCoreSystemPlugins(tempDir.name() + "/db");
		RandomSeedCache(catapultCache);

		auto supplementalData = CreateDeterministicSupplementalData();

		// Act: save the state
		constexpr auto SaveState = SaveStateToDirectoryWithCheckpointing;
		SaveState(dataDirectory, nodeConfig, catapultCache, supplementalData.State, supplementalData.ChainScore);

		// Assert:
		EXPECT_EQ(consumers::CommitOperationStep::All_Updated, ReadCommitStep(dataDirectory));
	}

	// endregion
}}
