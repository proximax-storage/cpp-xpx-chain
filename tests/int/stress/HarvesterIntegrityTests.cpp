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

#include "extensions/harvesting/src/Harvester.h"
#include "extensions/harvesting/src/HarvesterBlockGenerator.h"
#include "extensions/harvesting/src/HarvestingUtFacadeFactory.h"
#include "plugins/services/hashcache/src/cache/HashCacheStorage.h"
#include "plugins/services/hashcache/src/plugins/MemoryHashCacheSystem.h"
#include "catapult/cache/MemoryUtCache.h"
#include "catapult/cache/ReadOnlyCatapultCache.h"
#include "catapult/cache_core/BlockDifficultyCache.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/observers/NotificationObserverAdapter.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/local/RealTransactionFactory.h"
#include "tests/test/nodeps/Filesystem.h"
#include "tests/test/nodeps/TestConstants.h"
#include "tests/TestHarness.h"
#include <boost/thread.hpp>

namespace catapult { namespace harvesting {

#define TEST_CLASS HarvesterIntegrityTests

	namespace {
		uint64_t GetNumIterations() {
			return test::GetStressIterationCount() ? 5'000 : 250;
		}

		// region test factories

		std::shared_ptr<plugins::PluginManager> CreatePluginManager() {
			// include memory hash cache system to better trigger the race condition under test
			auto config = test::CreateLocalNodeBlockChainConfiguration();
			config.Plugins.emplace("catapult.plugins.transfer", utils::ConfigurationBag({{ "", { { "maxMessageSize", "0" } } }}));
			auto pPluginManager = test::CreatePluginManager(config);
			plugins::RegisterMemoryHashCacheSystem(*pPluginManager);
			return pPluginManager;
		}

		auto CreateConfiguration() {
			auto blockChainConfig = test::CreateLocalNodeBlockChainConfiguration();
			blockChainConfig.ShouldEnableVerifiableState = true;

			auto nodeConfig = config::NodeConfiguration::Uninitialized();
			nodeConfig.FeeInterest = 1;
			nodeConfig.FeeInterestDenominator = 2;

			return config::LocalNodeConfiguration {
				std::move(blockChainConfig),
				std::move(nodeConfig),
				config::LoggingConfiguration::Uninitialized(),
				config::UserConfiguration::Uninitialized()
			};
		}

		cache::CatapultCache CreateCatapultCache(const std::string& databaseDirectory, const model::BlockChainConfiguration& config) {
			auto cacheId = cache::HashCache::Id;
			auto cacheConfig = cache::CacheConfiguration(databaseDirectory, utils::FileSize(), cache::PatriciaTreeStorageMode::Enabled);

			std::vector<std::unique_ptr<cache::SubCachePlugin>> subCaches(cacheId + 1);
			test::CoreSystemCacheFactory::CreateSubCaches(config, subCaches);
			auto transactionCacheDuration = CalculateTransactionCacheDuration(config);
			subCaches[cacheId] = test::MakeSubCachePlugin<cache::HashCache, cache::HashCacheStorage>(transactionCacheDuration);
			return cache::CatapultCache(std::move(subCaches));
		}

		// endregion

		// region HarvesterTestContext

		class HarvesterTestContext {
		public:
			HarvesterTestContext()
					: m_pPluginManager(CreatePluginManager())
					, m_config(CreateConfiguration())
					, m_transactionsCache(cache::MemoryCacheOptions(1024, GetNumIterations() * 2))
					, m_cache(CreateCatapultCache(m_dbDirGuard.name(), m_config.BlockChain))
					, m_unlockedAccounts(100) {
				// create the harvester
				auto executionConfig = extensions::CreateExecutionConfiguration(*m_pPluginManager);
				HarvestingUtFacadeFactory utFacadeFactory(m_cache, m_config, executionConfig);

				auto strategy = model::TransactionSelectionStrategy::Oldest;
				auto blockGenerator = CreateHarvesterBlockGenerator(strategy, utFacadeFactory, m_transactionsCache);
				m_pHarvester = std::make_unique<Harvester>(m_cache, m_config, m_unlockedAccounts, blockGenerator);
			}

		public:
			cache::MemoryUtCache& transactionsCache() {
				return m_transactionsCache;
			}

			cache::CatapultCache& cache() {
				return m_cache;
			}

			Harvester& harvester() {
				return *m_pHarvester;
			}

		public:
			std::unique_ptr<model::Block> createLastBlock() {
				// create fake nemesis block
				auto pLastBlock = test::GenerateEmptyRandomBlock();
				pLastBlock->Height = Height(1);
				pLastBlock->Timestamp = Timestamp(1);
				pLastBlock->Difficulty = Difficulty(NEMESIS_BLOCK_DIFFICULTY);

				auto cacheDelta = m_cache.createDelta();
				auto& difficultyCache = cacheDelta.sub<cache::BlockDifficultyCache>();
				state::BlockDifficultyInfo difficultyInfo(pLastBlock->Height, pLastBlock->Timestamp, pLastBlock->Difficulty);
				difficultyCache.insert(difficultyInfo);
				m_cache.commit(Height(1));
				return pLastBlock;
			}

			void prepareAndUnlockSenderAccount(crypto::KeyPair&& keyPair) {
				// 1. seed an account with an initial currency balance of N and harvesting balance of 10'000'000
				auto currencyMosaicId = test::Default_Currency_Mosaic_Id;
				auto cacheDelta = m_cache.createDelta();
				auto& accountStateCacheDelta = cacheDelta.sub<cache::AccountStateCache>();
				accountStateCacheDelta.addAccount(keyPair.publicKey(), Height(1));
				auto accountStateIter = accountStateCacheDelta.find(keyPair.publicKey());
				accountStateIter.get().Balances.track(currencyMosaicId);
				accountStateIter.get().Balances.credit(currencyMosaicId, Amount(1000000000000));
				m_cache.commit(Height(1));

				// 2. unlock the account
				m_unlockedAccounts.modifier().add(std::move(keyPair));
			}

			void prepareSenderAccountAndTransactions(crypto::KeyPair&& keyPair, Timestamp deadline) {
				// 1. seed the UT cache with N txes
				auto recipient = test::GenerateRandomData<Key_Size>();
				for (auto i = 0u; i < GetNumIterations(); ++i) {
					auto pTransaction = test::CreateTransferTransaction(keyPair, recipient, Amount(1));
					pTransaction->MaxFee = Amount(0);
					pTransaction->Deadline = deadline;

					auto transactionHash = model::CalculateHash(*pTransaction);
					model::TransactionInfo transactionInfo(std::move(pTransaction), transactionHash);
					m_transactionsCache.modifier().add(std::move(transactionInfo));
				}

				// 2. seed and unlock an account with an initial balance of N
				prepareAndUnlockSenderAccount(std::move(keyPair));
			}

			void execute(const model::TransactionInfo& transactionInfo) {
				auto cacheDelta = m_cache.createDelta();
				auto catapultState = state::CatapultState();
				auto observerState = observers::ObserverState(cacheDelta, catapultState);

				// 4. prepare resolvers
				auto readOnlyCache = cacheDelta.toReadOnly();
				auto resolverContext = m_pPluginManager->createResolverContext(readOnlyCache);

				// 5. execute block
				auto notifyMode = observers::NotifyMode::Commit;
				observers::NotificationObserverAdapter entityObserver(
						m_pPluginManager->createObserver(),
						m_pPluginManager->createNotificationPublisher());
				auto observerContext = observers::ObserverContext(observerState, Height(1), notifyMode, resolverContext);
				entityObserver.notify(model::WeakEntityInfo(*transactionInfo.pEntity, transactionInfo.EntityHash), observerContext);
				m_cache.commit(Height(1));
			}

		private:
			test::TempDirectoryGuard m_dbDirGuard;
			std::shared_ptr<plugins::PluginManager> m_pPluginManager;
			config::LocalNodeConfiguration m_config;
			cache::MemoryUtCache m_transactionsCache;
			cache::CatapultCache m_cache;
			UnlockedAccounts m_unlockedAccounts;
			std::unique_ptr<Harvester> m_pHarvester;
		};

		// endregion
	}

	NO_STRESS_TEST(TEST_CLASS, HarvestIsThreadSafeWhenBlockDifficultyCacheIsChanging) {
		// Arrange:
		HarvesterTestContext context;
		auto pLastBlock = context.createLastBlock();

		// - seed and unlock a sender account
		context.prepareAndUnlockSenderAccount(test::GenerateKeyPair());

		// Act:
		// - let the harvester sometimes see a cache height 1 and sometimes height 2
		boost::thread_group threads;
		threads.create_thread([&context] {
			for (auto i = 0u; i < GetNumIterations(); ++i) {
				// 1. add block difficulty info and commit cache at height 2
				{
					auto cacheDelta = context.cache().createDelta();
					auto& difficultyCache = cacheDelta.sub<cache::BlockDifficultyCache>();
					state::BlockDifficultyInfo difficultyInfo(Height(2), Timestamp(i), Difficulty());
					difficultyCache.insert(difficultyInfo);
					context.cache().commit(Height(2));
				}

				// 2. wait a bit
				test::Sleep(1);

				// 3. remove block difficulty info and commit cache at height 1
				{
					auto cacheDelta = context.cache().createDelta();
					auto& difficultyCache = cacheDelta.sub<cache::BlockDifficultyCache>();
					difficultyCache.remove(Height(2));
					context.cache().commit(Height(1));
				}

				// 4. wait a bit
				test::Sleep(1);
			}
		});

		// - simulate harvester by harvesting blocks
		auto numHarvests = 0u;
		auto previousBlockElement = test::BlockToBlockElement(*pLastBlock);
		threads.create_thread([&context, &numHarvests, &previousBlockElement] {
			auto harvestTimestamp = previousBlockElement.Block.Timestamp + Timestamp(std::numeric_limits<int64_t>::max());
			for (auto i = 0u; i < GetNumIterations(); ++i) {
				auto pHarvestedBlock = context.harvester().harvest(previousBlockElement, harvestTimestamp);
				if (pHarvestedBlock)
					++numHarvests;

				test::Sleep(1);
			}
		});

		// - wait for all threads
		threads.join_all();

		CATAPULT_LOG(debug) << numHarvests << "/" << GetNumIterations() << " blocks harvested";

		// Assert: some harvesting attempts succeeded, some failed
		auto cacheView = context.cache().createView();
		EXPECT_LT(0u, numHarvests);
		EXPECT_GT(GetNumIterations(), numHarvests);
		EXPECT_EQ(1u, cacheView.sub<cache::BlockDifficultyCache>().size());
		EXPECT_EQ(0u, cacheView.sub<cache::HashCache>().size());
	}
}}
