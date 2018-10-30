/**
*** Copyright (c) 2016-present,
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
#include "catapult/chain/BlockExecutor.h"
#include "catapult/chain/ChainSynchronizer.h"
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/config/NodeConfiguration.h"
#include "catapult/consumers/BlockConsumers.h"
#include "catapult/disruptor/ConsumerDispatcher.h"
#include "catapult/disruptor/ConsumerInput.h"
#include "catapult/extensions/LocalNodeStateRef.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/model/BlockUtils.h"
#include "catapult/thread/MultiServicePool.h"
#include "catapult/utils/TimeSpan.h"
#include "catapult/utils/FileSize.h"
#include "catapult/validators/AggregateEntityValidator.h"
#include "extensions/sync/src/DispatcherService.h"
#include "extensions/sync/src/ExecutionConfigurationFactory.h"
#include "extensions/sync/src/PredicateUtils.h"
#include "MockRemoteChainApi.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "sync/src/RollbackInfo.h"
#include "tests/catapult/extensions/test/LocalNodeStateUtils.h"
#include "tests/test/core/BlockTestUtils.h"
#include "tests/test/core/mocks/MockTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/local/LocalTestUtils.h"
#include "tests/test/other/mocks/MockNodeSubscriber.h"
#include "tests/test/other/mocks/MockStateChangeSubscriber.h"
#include "tests/test/other/mocks/MockTransactionStatusSubscriber.h"
#include <memory>

namespace catapult { namespace sync {

#define TEST_CLASS ChainSyncTests

	namespace {
	constexpr auto Sync_Source = disruptor::InputSource::Remote_Pull;
	auto signer = crypto::KeyPair::FromString("A41BE076B942D915EA3330B135D35C5A959A2DCC50BBB393C6407984D4A3B564");

		chain::ChainSynchronizerConfiguration CreateChainSynchronizerConfiguration(const config::LocalNodeConfiguration& config) {
			chain::ChainSynchronizerConfiguration chainSynchronizerConfig;
			chainSynchronizerConfig.MaxBlocksPerSyncAttempt = config.Node.MaxBlocksPerSyncAttempt;
			chainSynchronizerConfig.MaxChainBytesPerSyncAttempt = config.Node.MaxChainBytesPerSyncAttempt.bytes32();
			chainSynchronizerConfig.MaxRollbackBlocks = config.BlockChain.MaxRollbackBlocks;
			return chainSynchronizerConfig;
		}

		config::LocalNodeConfiguration CreateLocalNodeConfiguration() {
			auto blockChainConfig = model::BlockChainConfiguration::Uninitialized();
			blockChainConfig.Network.Identifier = model::NetworkIdentifier::Mijin_Test;
			blockChainConfig.MaxRollbackBlocks = 25u;
			blockChainConfig.EffectiveBalanceRange = 100u;
			blockChainConfig.BlockGenerationTargetTime = utils::TimeSpan::FromSeconds(15u);
			blockChainConfig.MaxBlockFutureTime = utils::TimeSpan::FromSeconds(10u);
			blockChainConfig.MaxDifficultyBlocks = 4u;
			blockChainConfig.MaxTransactionsPerBlock = 200;

			auto nodeConfig = config::NodeConfiguration::Uninitialized();
			nodeConfig.MaxBlocksPerSyncAttempt = 30u;
			nodeConfig.MaxChainBytesPerSyncAttempt = utils::FileSize::FromMegabytes(1u);
			nodeConfig.ShouldAuditDispatcherInputs = false;
			nodeConfig.ShouldPrecomputeTransactionAddresses = false;
			nodeConfig.BlockDisruptorSize = 4096u;

			return config::LocalNodeConfiguration{
					std::move(blockChainConfig),
					std::move(nodeConfig),
					config::LoggingConfiguration::Uninitialized(),
					config::UserConfiguration::Uninitialized()
			};
		}

		std::shared_ptr<extensions::LocalNodeState> CreateLocalNodeState(
			const config::LocalNodeConfiguration& config) {
			return std::make_shared<extensions::LocalNodeState>(
				config,
				std::make_unique<mocks::MockMemoryBasedStorage>());
		}

		auto CreateRollbackInfo(
				const chain::TimeSupplier& timeSupplier,
				const model::BlockChainConfiguration& config) {
			auto rollbackDurationHalf = utils::TimeSpan::FromMilliseconds(
				config.BlockGenerationTargetTime.millis() * config.MaxRollbackBlocks / 2);
			auto ret = std::make_shared<RollbackInfo>(timeSupplier, rollbackDurationHalf);
			return ret;
		}

		consumers::BlockChainSyncHandlers::UndoBlockFunc CreateSyncUndoBlockHandler(
				const std::shared_ptr<const observers::EntityObserver>& pUndoObserver) {
			return [pUndoObserver](const auto& blockElement, const auto& state) {
				CATAPULT_LOG(debug) << "rolling back block at height " << blockElement.Block.Height;
				chain::RollbackBlock(blockElement, *pUndoObserver, state);
			};
		}

		consumers::BlockChainSyncHandlers CreateBlockChainSyncHandlers(extensions::ServiceState& state, RollbackInfo& rollbackInfo) {
			const auto& pluginManager = state.pluginManager();

			consumers::BlockChainSyncHandlers syncHandlers;
			syncHandlers.DifficultyChecker = [&rollbackInfo](const model::Block& remoteBlock, const model::Block& localBlock) {
				auto result = remoteBlock.CumulativeDifficulty > localBlock.CumulativeDifficulty;
				rollbackInfo.reset();
				return result;
			};

			auto undoBlockHandler = CreateSyncUndoBlockHandler(extensions::CreateUndoEntityObserver(pluginManager));
			syncHandlers.UndoBlock = [&rollbackInfo, undoBlockHandler](const auto& blockElement, const auto& observerState) {
				rollbackInfo.increment();
				undoBlockHandler(blockElement, observerState);
			};

			syncHandlers.BatchEntityProcessor = chain::CreateBatchEntityProcessor(CreateExecutionConfiguration(pluginManager));
			syncHandlers.StateChange = [&rollbackInfo, &subscriber = state.stateChangeSubscriber()](
					const auto& changeInfo) {
				subscriber.notifyStateChange(changeInfo);

				rollbackInfo.save();
			};

			syncHandlers.TransactionsChange = state.hooks().transactionsChangeHandler();
			return syncHandlers;
		}

		disruptor::ConsumerDispatcherOptions CreateBlockConsumerDispatcherOptions(const config::NodeConfiguration& config) {
			auto options = disruptor::ConsumerDispatcherOptions("block dispatcher", config.BlockDisruptorSize);
			options.ElementTraceInterval = config.BlockElementTraceInterval;
			options.ShouldThrowIfFull = config.ShouldAbortWhenDispatcherIsFull;
			return options;
		}

		cache::AccountStateCacheTypes::Options CreateAccountStateCacheOptions(const model::BlockChainConfiguration& config) {
			return { config.Network.Identifier, config.MinHarvesterBalance, config.EffectiveBalanceRange };
		}

		std::shared_ptr<model::Block> SeedBlocks(
				std::vector<io::BlockStorageCache*> vStorages,
				Height startHeight,
				Height endHeight,
				const model::BlockChainConfiguration& config,
				std::shared_ptr<model::Block> pParentBlock = nullptr) {
			for (auto height = startHeight; height <= endHeight; height = height + Height(1)) {
				model::PreviousBlockContext previousBlockContext{};
				if (!!pParentBlock) {
					previousBlockContext = model::PreviousBlockContext{test::BlockToBlockElement(*pParentBlock)};
				}
				model::BlockHitContext hitContext;
				hitContext.BaseTarget = 1 << 16;
				test::ConstTransactions transactions;
				auto pBlock = model::CreateBlock(previousBlockContext, hitContext, model::NetworkIdentifier::Mijin_Test, signer.publicKey(), transactions);
				pBlock->Timestamp = Timestamp{height.unwrap() * config.BlockGenerationTargetTime.millis()};
				pBlock->Height = height;
				pBlock->PreviousBlockHash = previousBlockContext.BlockHash;
				test::SignBlock(signer, *pBlock);
				auto blockElement = test::BlockToBlockElement(*pBlock);
				blockElement.GenerationHash = model::CalculateGenerationHash(
					previousBlockContext.BlockHash,
					signer.publicKey());
				for (auto pStorage : vStorages)
				{
					pStorage->modifier().saveBlock(blockElement);
				}
				pParentBlock = std::move(pBlock);
			}

			return std::move(pParentBlock);
		}

		void SynchronizeChains(
				const io::BlockStorageCache& remoteStorage,
				std::shared_ptr<extensions::LocalNodeState> pState,
				const config::LocalNodeConfiguration& config) {
			ionet::NodeContainer nodes;
			auto nodeStateRef = extensions::LocalNodeStateRef{*pState};
			auto pUtCache = test::CreateUtCacheProxy();
			supplier<Timestamp> timeSupplier = []() { return Timestamp(111000000000); };
			mocks::MockTransactionStatusSubscriber transactionStatusSubscriber;
			mocks::MockStateChangeSubscriber stateChangeSubscriber;
			mocks::MockNodeSubscriber nodeSubscriber;
			std::vector<utils::DiagnosticCounter> counters;
			plugins::PluginManager pluginManager(config.BlockChain, plugins::StorageConfiguration());
			pluginManager.addCacheSupport<cache::AccountStateCacheStorage>(std::make_unique<cache::AccountStateCache>(
				cache::CacheConfiguration(),
				CreateAccountStateCacheOptions(config.BlockChain)));
			nodeStateRef.Cache = pluginManager.createCache();
			thread::MultiServicePool pool("test", 1);

			auto serviceState = extensions::ServiceState(
				nodeStateRef,
				nodes,
				*pUtCache,
				timeSupplier,
				transactionStatusSubscriber,
				stateChangeSubscriber,
				nodeSubscriber,
				counters,
				pluginManager,
				pool);

			auto pValidatorPool = serviceState.pool().pushIsolatedPool("validator", 1);
			auto pRollbackInfo = CreateRollbackInfo(serviceState.timeSupplier(), config.BlockChain);

			std::vector<disruptor::BlockConsumer> vConsumers;
			vConsumers.push_back(consumers::CreateBlockStatelessValidationConsumer(
					extensions::CreateStatelessValidator(pluginManager),
					validators::CreateParallelValidationPolicy(pValidatorPool),
					ToUnknownTransactionPredicate(serviceState.hooks().knownHashPredicate(serviceState.utCache()))));

			auto disruptorConsumers = disruptor::DisruptorConsumersFromBlockConsumers(vConsumers);
			disruptorConsumers.push_back(consumers::CreateBlockChainSyncConsumer(
					serviceState.nodeLocalState(),
					CreateBlockChainSyncHandlers(serviceState, *pRollbackInfo)));

			disruptorConsumers.push_back(consumers::CreateNewBlockConsumer(serviceState.hooks().newBlockSink(), disruptor::InputSource::Local));

			auto pDispatcher = std::make_unique<disruptor::ConsumerDispatcher>(CreateBlockConsumerDispatcherOptions(config.Node), disruptorConsumers);

			auto blockRangeConsumer = [&dispatcher = *pDispatcher](auto&& range, const auto& processingComplete) {
				return dispatcher.processElement(disruptor::ConsumerInput(std::move(range), Sync_Source), processingComplete);
			};

			auto chainSynchronizer = chain::CreateChainSynchronizer(
					api::CreateLocalChainApi(
							pState->Storage,
							config.Node.MaxBlocksPerSyncAttempt),
					CreateChainSynchronizerConfiguration(config),
					blockRangeConsumer);

			mocks::MockRemoteChainApi remoteApi{
				remoteStorage,
				config.Node.MaxBlocksPerSyncAttempt};

			chainSynchronizer(remoteApi).get();

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		std::shared_ptr<model::Block> PopulateCommonBlocks(io::BlockStorageCache& localStorage, io::BlockStorageCache& remoteStorage,
				const config::LocalNodeConfiguration& config, Height endHeight) {
			localStorage.modifier().dropBlocksAfter(Height{0});
			remoteStorage.modifier().dropBlocksAfter(Height{0});
			auto startHeight = Height{1u};
			return SeedBlocks({ &localStorage, &remoteStorage }, startHeight, endHeight, config.BlockChain);
		}

		void PopulateLocalAndRemoteBlocks(io::BlockStorageCache& localStorage, io::BlockStorageCache& remoteStorage,
				const config::LocalNodeConfiguration& config) {
			auto endHeight = Height{5u};
			auto pLastBlock = PopulateCommonBlocks(localStorage, remoteStorage, config, endHeight);

			auto startHeight = Height{endHeight.unwrap() + 1u};
			auto localEndHeight = Height{startHeight.unwrap() + 1u};
			auto remoteEndHeight = Height{startHeight.unwrap() + 2u};
			SeedBlocks({ &localStorage }, startHeight, localEndHeight, config.BlockChain, pLastBlock);
			SeedBlocks({ &remoteStorage }, startHeight, remoteEndHeight, config.BlockChain, pLastBlock);
		}
	}

	TEST(TEST_CLASS, SynchronizeChains) {
		// Act:
		auto config = CreateLocalNodeConfiguration();
		auto pState = CreateLocalNodeState(std::move(config));
		io::BlockStorageCache remoteStorage{std::make_unique<mocks::MockMemoryBasedStorage>()};
		PopulateLocalAndRemoteBlocks(pState->Storage, remoteStorage, config);
		SynchronizeChains(remoteStorage, pState, config);

		// Assert:
		ASSERT_NE(pState->Storage.view().chainHeight(), remoteStorage.view().chainHeight());
	}
}}
