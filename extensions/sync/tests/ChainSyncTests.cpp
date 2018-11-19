/**
*** Copyright (c) 2018-present,
*** PROXIMAX LIMITED. All rights reserved.
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

#include "catapult/api/LocalChainApi.h"
#include "catapult/cache_core/AccountStateCacheStorage.h"
#include "catapult/cache_core/BlockDifficultyCacheStorage.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/chain/BlockDifficultyScorer.h"
#include "catapult/chain/ChainSynchronizer.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/ServerHooks.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/utils/TimeSpan.h"
#include "catapult/utils/FileSize.h"
#include "extensions/sync/src/DispatcherService.h"
#include "MockRemoteChainApi.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockMemoryBasedStorage.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/nodeps/MijinConstants.h"

namespace catapult { namespace sync {

#define TEST_CLASS ChainSyncTests

	namespace {
		auto Special_Account_Key_Pair = crypto::KeyPair::FromString(test::Mijin_Test_Private_Keys[0]);
		auto Nemesis_Account_Key_Pair = crypto::KeyPair::FromString(test::Mijin_Test_Private_Keys[1]);
		uint64_t constexpr Initial_Balance = 1'000'000'000'000'000u;

		void InitializeCatapultCache(cache::CatapultCache& cache) {
			auto delta = cache.createDelta();

			auto& specialAccountState = delta.sub<cache::AccountStateCache>().addAccount(
				Special_Account_Key_Pair.publicKey(), Height{1});
			specialAccountState.Balances.credit(Xpx_Id, Amount(Initial_Balance), Height{1});

			auto& nemesisAccountState = delta.sub<cache::AccountStateCache>().addAccount(
				Nemesis_Account_Key_Pair.publicKey(), Height{1});
			nemesisAccountState.Balances.credit(Xpx_Id, Amount(Initial_Balance), Height{1});

			cache.commit(Height{1});
		}

		chain::ChainSynchronizerConfiguration CreateChainSynchronizerConfiguration(
				const config::LocalNodeConfiguration& config) {
			chain::ChainSynchronizerConfiguration chainSynchronizerConfig;
			chainSynchronizerConfig.MaxBlocksPerSyncAttempt = config.Node.MaxBlocksPerSyncAttempt;
			chainSynchronizerConfig.MaxChainBytesPerSyncAttempt = config.Node.MaxChainBytesPerSyncAttempt.bytes32();
			chainSynchronizerConfig.MaxRollbackBlocks = config.BlockChain.MaxRollbackBlocks;
			return chainSynchronizerConfig;
		}

		config::LocalNodeConfiguration CreateLocalNodeConfiguration() {
			auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
			blockChainConfig.Network.Identifier = model::NetworkIdentifier::Mijin_Test;
			blockChainConfig.MaxRollbackBlocks = 5u;
			blockChainConfig.ImportanceGrouping = 10u;
			blockChainConfig.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(15u);
			blockChainConfig.MaxBlockFutureTime = utils::TimeSpan::FromSeconds(10u);
			blockChainConfig.MaxDifficultyBlocks = 4u;

			auto nodeConfig = config::NodeConfiguration::Uninitialized();
			nodeConfig.MaxBlocksPerSyncAttempt = 30u;
			nodeConfig.MaxChainBytesPerSyncAttempt = utils::FileSize::FromMegabytes(1u);

			return config::LocalNodeConfiguration{
					std::move(blockChainConfig),
					std::move(nodeConfig),
					config::LoggingConfiguration::Uninitialized(),
					config::UserConfiguration::Uninitialized()
			};
		}

		void SynchronizeChains(
				chain::CompletionAwareBlockRangeConsumerFunc& blockRangeConsumer,
				extensions::ServiceState& serviceState,
				const io::BlockStorageCache& remoteStorage,
				const config::LocalNodeConfiguration& config) {
			auto chainSynchronizer = chain::CreateChainSynchronizer(
				api::CreateLocalChainApi(
					serviceState.storage(),
					[&score = serviceState.score()]() { return score.get(); },
					config.Node.MaxBlocksPerSyncAttempt),
				CreateChainSynchronizerConfiguration(config),
				blockRangeConsumer);

			extensions::LocalNodeChainScore remoteChainScore{model::ChainScore{2000u}};
			mocks::MockRemoteChainApi remoteApi{
				remoteStorage,
				config.Node.MaxBlocksPerSyncAttempt,
				remoteChainScore};

			chainSynchronizer(remoteApi).get();

			std::this_thread::sleep_for(std::chrono::milliseconds(100u));
		}

		auto CreatePreviousBlockContext(const std::shared_ptr<model::Block>& pBlock) {
			return !pBlock ?
				model::PreviousBlockContext{} :
				model::PreviousBlockContext{test::BlockToBlockElement(*pBlock)};
		}

		std::shared_ptr<model::Block> CreateBlock(
				cache::CatapultCache& cache,
				const Height& height,
				Timestamp timestamp,
				const model::BlockChainConfiguration& config,
				const std::shared_ptr<model::Block>& pParentBlock,
				const crypto::KeyPair& signer) {
			model::PreviousBlockContext previousBlockContext = CreatePreviousBlockContext(pParentBlock);
			test::ConstTransactions transactions{};
			auto pBlock = model::CreateBlock(previousBlockContext, model::NetworkIdentifier::Mijin_Test,
					signer.publicKey(), transactions);
			pBlock->Height = height;
			pBlock->Timestamp = timestamp;
			if (pParentBlock)
				chain::TryCalculateDifficulty(cache.sub<cache::BlockDifficultyCache>(),
					pParentBlock->Height, config, pBlock->Difficulty);
			test::SignBlock(signer, *pBlock);

			auto blockElement = test::BlockToBlockElement(*pBlock);
			blockElement.GenerationHash = model::CalculateGenerationHash(
				previousBlockContext.BlockHash,
				signer.publicKey());
			blockElement.EntityHash = model::CalculateHash(*pBlock);

			return pBlock;
		}

		std::shared_ptr<model::Block> PopulateCommonBlocks(
				const std::vector<io::BlockStorageCache*>& vStorages,
				cache::CatapultCache& cache,
				const model::BlockChainConfiguration& config,
				Height endHeight) {
			std::shared_ptr<model::Block> pLastBlock{};
			for (auto height = Height{1u}; height <= endHeight; height = height + Height{1u}) {
				auto timestamp = Timestamp{height.unwrap() * config.BlockGenerationTargetTime.millis()};
				const auto& signer = height.unwrap() % 2 ?
					Nemesis_Account_Key_Pair : Special_Account_Key_Pair;
				pLastBlock = CreateBlock(cache, height, timestamp, config, pLastBlock, signer);

				auto delta = cache.createDelta();
				delta.sub<cache::BlockDifficultyCache>().insert(
					pLastBlock->Height, pLastBlock->Timestamp, pLastBlock->Difficulty);
				cache.commit(pLastBlock->Height);

				for (auto pStorage : vStorages) {
					pStorage->modifier().saveBlock(test::BlockToBlockElement(*pLastBlock));
				}
			}

			return pLastBlock;
		}

		struct DispatcherServiceTraits {
			static constexpr auto CreateRegistrar = CreateDispatcherServiceRegistrar;
		};

		class TestContext : public test::ServiceLocatorTestContext<DispatcherServiceTraits> {
		private:
			using BaseType = test::ServiceLocatorTestContext<DispatcherServiceTraits>;

		public:
			TestContext(const model::BlockChainConfiguration& config)
				: BaseType(test::CreateEmptyCatapultCache<test::CoreSystemCacheFactory>(config)) {
				// initialize the cache
				InitializeCatapultCache(testState().state().cache());
			}

			Importance getAccountImportanceOrDefault(const Key& publicKey, Height height) {
				const auto& accountStateCache = testState().state().cache().sub<cache::AccountStateCache>();
				auto view = accountStateCache.createView();
				cache::ReadOnlyAccountStateCache readOnlyCache(*view);
				cache::ImportanceView importanceView(readOnlyCache);
				return importanceView.getAccountImportanceOrDefault(publicKey, height);
			}
		};
	}

	TEST(TEST_CLASS, DoubleSpend) {
		// Arrange: build common chain, then add a block to the local chain where
		// the special (malicious) account spends half his funds. After that the
		// special account adds a block to the remote chain (on its own node) at
		// the same height, but without funds transfer, and pushes it to the local chain.
		// Check that the remote chain is not accepted and the special account balance
		// remains half of the initial value.

		auto config = CreateLocalNodeConfiguration();

		io::BlockStorageCache remoteStorage{std::make_unique<mocks::MockMemoryBasedStorage>()};
		remoteStorage.modifier().dropBlocksAfter(Height{0u});

		TestContext context{config.BlockChain};
		context.boot();

		auto& state = context.testState().state();
		state.storage().modifier().dropBlocksAfter(Height{0u});
		auto blockRangeConsumer = state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Remote_Pull);

		auto endHeight = Height{50u};
		auto pLastBlock = PopulateCommonBlocks(
			{ &state.storage(), &remoteStorage },
			state.cache(),
			config.BlockChain,
			endHeight);

		endHeight = endHeight + Height{1u};
		auto timestamp = Timestamp{endHeight.unwrap() * config.BlockChain.BlockGenerationTargetTime.millis()};
		auto pLocalBlock = CreateBlock(
			state.cache(),
			endHeight,
			timestamp,
			config.BlockChain,
			pLastBlock,
			Nemesis_Account_Key_Pair);
		state.storage().modifier().saveBlock(test::BlockToBlockElement(*pLocalBlock));

		{
			auto delta = state.cache().createDelta();
			auto& cache = delta.sub<cache::AccountStateCache>();
			auto& accountState = cache.get(Special_Account_Key_Pair.publicKey());
			accountState.Balances.debit(Xpx_Id, Amount(Initial_Balance / 2u), endHeight);
			state.cache().commit(endHeight);
		}

		timestamp = Timestamp{timestamp.unwrap() - 1000};
		auto pRemoteBlock = CreateBlock(
			state.cache(),
			endHeight,
			timestamp,
			config.BlockChain,
			pLastBlock,
			Special_Account_Key_Pair);
		remoteStorage.modifier().saveBlock(test::BlockToBlockElement(*pRemoteBlock));

		// Act:
		SynchronizeChains(blockRangeConsumer, state, remoteStorage, config);

		// Assert:
		auto importance = context.getAccountImportanceOrDefault(Special_Account_Key_Pair.publicKey(), endHeight);
		ASSERT_EQ(importance.unwrap(), Initial_Balance / 2u);
	}
}}
