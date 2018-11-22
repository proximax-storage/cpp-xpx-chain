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
#include "catapult/chain/BlockExecutor.h"
#include "catapult/chain/ChainSynchronizer.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/extensions/ServerHooks.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/io/BlockStorageCache.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/plugins/PluginLoader.h"
#include "extensions/sync/src/DispatcherService.h"
#include "MockRemoteChainApi.h"
#include "sdk/src/builders/TransferBuilder.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "tests/test/cache/CacheTestUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockMemoryBasedStorage.h"
#include "tests/test/local/ServiceLocatorTestContext.h"
#include "tests/test/nodeps/MijinConstants.h"

namespace catapult { namespace sync {

#define TEST_CLASS ChainSyncTests

	namespace {
		auto Special_Account_Key_Pair = crypto::KeyPair::FromString(test::Mijin_Test_Private_Keys[0]);
		auto Nemesis_Account_Key_Pair = crypto::KeyPair::FromString(test::Mijin_Test_Private_Keys[1]);
		uint64_t constexpr Initial_Balance = 1'000'000'000'000'000u;

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
			blockChainConfig.BlockPruneInterval = 360u;
			blockChainConfig.Plugins.emplace("catapult.plugins.transfer", utils::ConfigurationBag({{ "", { { "maxMessageSize", "0" } } }}));

			auto nodeConfig = config::NodeConfiguration::Uninitialized();
			nodeConfig.MaxBlocksPerSyncAttempt = 30u;
			nodeConfig.MaxChainBytesPerSyncAttempt = utils::FileSize::FromMegabytes(1u);
			nodeConfig.BlockDisruptorSize = 4096u;
			nodeConfig.TransactionDisruptorSize = 16384u;

			return config::LocalNodeConfiguration{
					std::move(blockChainConfig),
					std::move(nodeConfig),
					config::LoggingConfiguration::Uninitialized(),
					config::UserConfiguration::Uninitialized()
			};
		}

		struct DispatcherServiceTraits {
			static constexpr auto CreateRegistrar = CreateDispatcherServiceRegistrar;
		};

		class TestContext : public test::ServiceLocatorTestContext<DispatcherServiceTraits> {
		private:
			using BaseType = test::ServiceLocatorTestContext<DispatcherServiceTraits>;

		public:
			TestContext(config::LocalNodeConfiguration&& config)
				: BaseType(test::CreateEmptyCatapultCache<test::CoreSystemCacheFactory>(config.BlockChain), std::move(config)) {

				testState().loadPluginByName("", "catapult.coresystem");
				for (const auto& pair : config.BlockChain.Plugins)
					testState().loadPluginByName("", pair.first);

				testState().state().storage().modifier().dropBlocksAfter(Height{0u});

				initializeCache();
			}

			void initializeCache() {
				auto delta = testState().state().cache().createDelta();

				auto& specialAccountState = delta.sub<cache::AccountStateCache>().addAccount(
					Special_Account_Key_Pair.publicKey(), Height{1});
				specialAccountState.Balances.credit(Xpx_Id, Amount(Initial_Balance), Height{1});

				auto& nemesisAccountState = delta.sub<cache::AccountStateCache>().addAccount(
					Nemesis_Account_Key_Pair.publicKey(), Height{1});
				nemesisAccountState.Balances.credit(Xpx_Id, Amount(Initial_Balance), Height{1});

				testState().state().cache().commit(Height{1});
			}

			Importance getAccountImportanceOrDefault(const Key& publicKey, Height height) {
				const auto& accountStateCache = testState().state().cache().sub<cache::AccountStateCache>();
				auto view = accountStateCache.createView();
				cache::ReadOnlyAccountStateCache readOnlyCache(*view);
				cache::ImportanceView importanceView(readOnlyCache);
				return importanceView.getAccountImportanceOrDefault(publicKey, height);
			}

			void executeBlock(const model::BlockElement& blockElement) {
				auto& state = testState().state();
				auto delta = state.cache().createDelta();
				auto observerState = observers::ObserverState(delta, state.state());
				auto pRootObserver = extensions::CreateEntityObserver(state.pluginManager());
				chain::ExecuteBlock(blockElement, *pRootObserver, observerState);
				state.cache().commit(blockElement.Block.Height);

				testState().state().storage().modifier().saveBlock(blockElement);
			}

			void synchronizeChains(const io::BlockStorageCache& remoteStorage) {
				auto& state = testState().state();
				const auto& config = state.config();
				auto chainSynchronizer = chain::CreateChainSynchronizer(
					api::CreateLocalChainApi(
						state.storage(),
						[&score = state.score()]() { return score.get(); },
						config.Node.MaxBlocksPerSyncAttempt),
					CreateChainSynchronizerConfiguration(config),
					state.hooks().completionAwareBlockRangeConsumerFactory()(disruptor::InputSource::Remote_Pull));

				extensions::LocalNodeChainScore remoteChainScore{model::ChainScore{2000u}};
				mocks::MockRemoteChainApi remoteApi{
					remoteStorage,
					config.Node.MaxBlocksPerSyncAttempt,
					remoteChainScore};

				chainSynchronizer(remoteApi).get();

				std::this_thread::sleep_for(std::chrono::milliseconds(100u));
			}

			model::BlockElement createBlock(
					const Height& height,
					Timestamp timestamp,
					const model::PreviousBlockContext& previousBlockContext,
					const crypto::KeyPair& signer,
					const test::ConstTransactions& transactions) {
				m_pLastBlock = model::CreateBlock(previousBlockContext, model::NetworkIdentifier::Mijin_Test,
						signer.publicKey(), transactions);
				m_pLastBlock->Height = height;
				m_pLastBlock->Timestamp = timestamp;
				chain::TryCalculateDifficulty(testState().state().cache().sub<cache::BlockDifficultyCache>(),
					previousBlockContext.BlockHeight, testState().state().config().BlockChain, m_pLastBlock->Difficulty);
				test::SignBlock(signer, *m_pLastBlock);

				auto blockElement = test::BlockToBlockElement(*m_pLastBlock);
				blockElement.GenerationHash = model::CalculateGenerationHash(
					previousBlockContext.BlockHash,
					signer.publicKey());

				return blockElement;
			}

			model::PreviousBlockContext populateCommonBlocks(
					io::BlockStorageCache& remoteStorage,
					Height endHeight) {
				model::PreviousBlockContext previousBlockContext{};
				previousBlockContext.BlockHeight = Height{1};
				for (auto height = Height{1u}; height <= endHeight; height = height + Height{1u}) {
					auto timestamp = Timestamp{height.unwrap() *
						testState().state().config().BlockChain.BlockGenerationTargetTime.millis()};
					const auto& signer = height.unwrap() % 2 ?
						Nemesis_Account_Key_Pair : Special_Account_Key_Pair;
					test::ConstTransactions transactions{};
					auto blockElement = createBlock(height, timestamp, previousBlockContext, signer, transactions);
					previousBlockContext = model::PreviousBlockContext{blockElement};

					executeBlock(blockElement);

					remoteStorage.modifier().saveBlock(blockElement);
				}

				return previousBlockContext;
			}

		private:
			std::unique_ptr<model::Block> m_pLastBlock;
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

		TestContext context(std::move(config));
		context.boot();

		io::BlockStorageCache remoteStorage{std::make_unique<mocks::MockMemoryBasedStorage>()};
		remoteStorage.modifier().dropBlocksAfter(Height{0u});

		Height endHeight(50u);
		auto previousBlockContext = context.populateCommonBlocks(remoteStorage, endHeight);

		auto nemesisAccountAddress = model::PublicKeyToAddress(Nemesis_Account_Key_Pair.publicKey(),
			config.BlockChain.Network.Identifier);
		builders::TransferBuilder builder(
			config.BlockChain.Network.Identifier,
			Special_Account_Key_Pair.publicKey() /* signer */,
			nemesisAccountAddress /* recipient */
		);
		builder.addMosaic(Xpx_Id, Amount(Initial_Balance / 2u));

		test::ConstTransactions localTransactions{};
		auto pTransaction = builder.build();
		extensions::SignTransaction(Special_Account_Key_Pair, *pTransaction);
		localTransactions.push_back(std::move(pTransaction));

		endHeight = endHeight + Height{1u};
		uint64_t timestampOfLocalChain(endHeight.unwrap() * config.BlockChain.BlockGenerationTargetTime.millis());
		auto localBlockElement = context.createBlock(
			endHeight,
			Timestamp(timestampOfLocalChain),
			previousBlockContext,
			Nemesis_Account_Key_Pair,
			localTransactions);

		// Remote block has less timestamp, so his difficulty is greater
		auto remoteBlockElement = context.createBlock(
			endHeight,
			Timestamp(timestampOfLocalChain - 1000),
			previousBlockContext,
			Special_Account_Key_Pair,
			test::ConstTransactions());

		remoteStorage.modifier().saveBlock(remoteBlockElement);
		context.executeBlock(localBlockElement);

		// Act:
		context.synchronizeChains(remoteStorage);

		// Assert:
		auto importance = context.getAccountImportanceOrDefault(Special_Account_Key_Pair.publicKey(), endHeight);
		ASSERT_EQ(importance.unwrap(), Initial_Balance / 2u);
	}
}}
