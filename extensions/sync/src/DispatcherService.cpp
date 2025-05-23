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

#include "DispatcherService.h"
#include "DispatcherSyncHandlers.h"
#include "PredicateUtils.h"
#include "RollbackInfo.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/BlockDifficultyCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/chain/BlockExecutor.h"
#include "catapult/chain/BlockScorer.h"
#include "catapult/chain/ChainUtils.h"
#include "catapult/config/CatapultDataDirectory.h"
#include "catapult/consumers/AuditConsumer.h"
#include "catapult/consumers/ConsumerUtils.h"
#include "catapult/consumers/ReclaimMemoryInspector.h"
#include "catapult/consumers/UndoBlock.h"
#include "catapult/extensions/DispatcherUtils.h"
#include "catapult/extensions/ExecutionConfigurationFactory.h"
#include "catapult/extensions/LocalNodeChainScore.h"
#include "catapult/extensions/NodeInteractionUtils.h"
#include "catapult/extensions/PluginUtils.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/notification_handlers/HandlerContext.h"
#include "catapult/subscribers/StateChangeSubscriber.h"
#include "catapult/subscribers/TransactionStatusSubscriber.h"
#include "catapult/thread/MultiServicePool.h"
#include "catapult/validators/AggregateEntityValidator.h"

using namespace catapult::consumers;
using namespace catapult::disruptor;

namespace catapult { namespace sync {

	namespace {
		// region utils

		std::vector<model::TransactionInfo> SelectValid(
				std::vector<model::TransactionInfo>&& transactionInfos,
				const std::vector<chain::UtUpdateResult>& updateResults) {

			std::vector<model::TransactionInfo> filteredTransactionInfos;

			for (auto i = 0u; i < transactionInfos.size(); ++i) {
				switch (updateResults[i].Type) {
				case chain::UtUpdateResult::UpdateType::Invalid:
				case chain::UtUpdateResult::UpdateType::Neutral:
					break;

				default:
					filteredTransactionInfos.push_back(std::move(transactionInfos[i]));
					break;
				}
			}

			return filteredTransactionInfos;
		}

		ConsumerDispatcherOptions CreateBlockConsumerDispatcherOptions(const config::NodeConfiguration& config) {
			auto options = ConsumerDispatcherOptions("block dispatcher", config.BlockDisruptorSize);
			options.ElementTraceInterval = config.BlockElementTraceInterval;
			options.ShouldThrowWhenFull = config.ShouldAbortWhenDispatcherIsFull;
			return options;
		}

		ConsumerDispatcherOptions CreateTransactionConsumerDispatcherOptions(const config::NodeConfiguration& config) {
			auto options = ConsumerDispatcherOptions("transaction dispatcher", config.TransactionDisruptorSize);
			options.ElementTraceInterval = config.TransactionElementTraceInterval;
			options.ShouldThrowWhenFull = config.ShouldAbortWhenDispatcherIsFull;
			return options;
		}

		std::unique_ptr<ConsumerDispatcher> CreateConsumerDispatcher(
				extensions::ServiceState& state,
				const ConsumerDispatcherOptions& options,
				std::vector<DisruptorConsumer>&& disruptorConsumers) {
			auto& statusSubscriber = state.transactionStatusSubscriber();
			auto reclaimMemoryInspector = CreateReclaimMemoryInspector();
			auto inspector = [&statusSubscriber, &nodes = state.nodes(), reclaimMemoryInspector](
					auto& input,
					const auto& completionResult) {
				statusSubscriber.flush();
				auto interactionResult = consumers::ToNodeInteractionResult(input.sourcePublicKey(), completionResult);
				extensions::IncrementNodeInteraction(nodes, interactionResult);
				reclaimMemoryInspector(input, completionResult);
			};

			// if enabled, add an audit consumer before all other consumers
			const auto& config = state.config();
			if (config.Node.ShouldAuditDispatcherInputs) {
				auto auditPath = boost::filesystem::path(config.User.DataDirectory) / "audit" / std::string(options.DispatcherName);
				auditPath /= std::to_string(state.timeSupplier()().unwrap());
				CATAPULT_LOG(debug) << "enabling auditing to " << auditPath;

				boost::filesystem::create_directories(auditPath);
				disruptorConsumers.insert(disruptorConsumers.begin(), CreateAuditConsumer(auditPath.generic_string()));
			}

			return std::make_unique<ConsumerDispatcher>(options, disruptorConsumers, inspector);
		}

		// endregion

		// region block

		BlockChainProcessor CreateSyncProcessor(
				extensions::ServiceState& state,
				const chain::ExecutionConfiguration& executionConfig) {
			BlockHitPredicateFactory blockHitPredicateFactory = [&state](const cache::ReadOnlyCatapultCache& cache) {
				cache::ImportanceView view(cache.sub<cache::AccountStateCache>());
				return chain::BlockHitPredicate(state.pluginManager().configHolder(), [view](const auto& publicKey, auto height) {
					return view.getAccountImportanceOrDefault(publicKey, height);
				});
			};
			return CreateBlockChainProcessor(
					blockHitPredicateFactory,
					chain::CreateBatchEntityProcessor(executionConfig),
					state);
		}

		BlockChainSyncHandlers CreateBlockChainSyncHandlers(extensions::ServiceState& state, RollbackInfo& rollbackInfo) {
			const auto& pluginManager = state.pluginManager();

			BlockChainSyncHandlers syncHandlers;
			syncHandlers.DifficultyChecker = [&rollbackInfo, &pluginManager](
					const auto& blocks,
					const cache::CatapultCache& cache,
					const model::NetworkConfigurations& remoteConfigs) {
				if (!blocks.size())
					return true;
				auto result = chain::CheckDifficulties(cache.sub<cache::BlockDifficultyCache>(), blocks, pluginManager.configHolder(), remoteConfigs);
				rollbackInfo.modifier().reset();
				return blocks.size() == result;
			};

			auto pUndoObserver = utils::UniqueToShared(extensions::CreateUndoEntityObserver(pluginManager));
			syncHandlers.UndoBlock = [&rollbackInfo, &pluginManager, pUndoObserver](
					const model::NetworkConfiguration& config,
					const auto& blockElement,
					auto& observerState,
					auto undoBlockType) {
				rollbackInfo.modifier().increment();
				auto readOnlyCache = observerState.Cache.toReadOnly();
				auto resolverContext = pluginManager.createResolverContext(readOnlyCache);
				UndoBlock(config, blockElement, { *pUndoObserver, resolverContext, pluginManager.configHolder(), observerState }, undoBlockType);
			};
			syncHandlers.Processor = CreateSyncProcessor(state, extensions::CreateExecutionConfiguration(pluginManager));

			syncHandlers.StateChange = [&rollbackInfo, &localScore = state.score(), &subscriber = state.stateChangeSubscriber()](
					const auto& changeInfo) {
				localScore += changeInfo.ScoreDelta;

				// note: changeInfo contains only score delta, subscriber will get both current local score and changeInfo
				subscriber.notifyScoreChange(localScore.get());
				subscriber.notifyStateChange(changeInfo);

				rollbackInfo.modifier().save();
			};

			auto dataDirectory = config::CatapultDataDirectory(state.config().User.DataDirectory);
			syncHandlers.PreStateWritten = [](const auto&, const auto&, auto) {};
			syncHandlers.TransactionsChange = state.hooks().transactionsChangeHandler();
			syncHandlers.CommitStep = CreateCommitStepHandler(dataDirectory);
			syncHandlers.PostBlockCommit = [&state](const std::vector<model::BlockElement>& elements) {
				for (const auto& element : elements)
					state.postBlockCommitSubscriber().notifyBlock(element);
			};
			syncHandlers.PostBlockCommitNotifications = [&state](const model::BlockElement& blockElement, const std::vector<std::unique_ptr<model::Notification>>& notifications) {
				const auto& notificationSubscriber = state.notificationSubscriber();
				auto handlerContext = notification_handlers::HandlerContext(
						state.config(blockElement.Block.Height),
						blockElement.Block.Height,
						blockElement.Block.Timestamp);
				for (const auto& pNotification : notifications)
					notificationSubscriber.handle(*pNotification, handlerContext);
			};

			if (state.config().Node.ShouldUseCacheDatabaseStorage)
				AddSupplementalDataResiliency(syncHandlers, dataDirectory, state.cache(), state.score());

			return syncHandlers;
		}

		class BlockDispatcherBuilder {
		public:
			explicit BlockDispatcherBuilder(extensions::ServiceState& state)
					: m_state(state)
					, m_nodeConfig(m_state.config().Node)
			{}

		public:
			void addHashConsumers() {
				m_consumers.push_back(CreateBlockHashCalculatorConsumer(
						m_state.config().Immutable.GenerationHash,
						m_state.pluginManager().transactionRegistry()));
				m_consumers.push_back(CreateBlockHashCheckConsumer(
						m_state.timeSupplier(),
						extensions::CreateHashCheckOptions(m_nodeConfig.ShortLivedCacheBlockDuration, m_nodeConfig)));
			}

			std::shared_ptr<ConsumerDispatcher> build(
					const std::shared_ptr<thread::IoThreadPool>& pValidatorPool,
					RollbackInfo& rollbackInfo) {
				const auto& pluginManager = m_state.pluginManager();
				m_consumers.push_back(CreateBlockChainCheckConsumer(
						m_nodeConfig.MaxBlocksPerSyncAttempt,
						pluginManager.configHolder(),
						m_state.timeSupplier()));
				m_consumers.push_back(CreateBlockStatelessValidationConsumer(
						extensions::CreateStatelessValidator(pluginManager),
						validators::CreateParallelValidationPolicy(pValidatorPool),
						ToRequiresValidationPredicate(m_state.hooks().knownHashPredicate(m_state.utCache()))));

				auto disruptorConsumers = DisruptorConsumersFromBlockConsumers(m_consumers);
				disruptorConsumers.push_back(CreateBlockChainSyncConsumer(
						m_state.cache(),
						m_state.state(),
						m_state.storage(),
						pluginManager.configHolder(),
						CreateBlockChainSyncHandlers(m_state, rollbackInfo)));

				if (m_state.config().Node.ShouldEnableAutoSyncCleanup)
					disruptorConsumers.push_back(CreateBlockChainSyncCleanupConsumer(m_state.config().User.DataDirectory));

				disruptorConsumers.push_back(CreateNewBlockConsumer(m_state.hooks().newBlockSink(), InputSource::Local));
				return CreateConsumerDispatcher(
						m_state,
						CreateBlockConsumerDispatcherOptions(m_nodeConfig),
						std::move(disruptorConsumers));
			}

		private:
			extensions::ServiceState& m_state;
			config::NodeConfiguration m_nodeConfig;
			std::vector<BlockConsumer> m_consumers;
		};

		void RegisterBlockDispatcherService(
				const std::shared_ptr<ConsumerDispatcher>& pDispatcher,
				thread::MultiServicePool::ServiceGroup& serviceGroup,
				extensions::ServiceLocator& locator,
				extensions::ServiceState& state) {
			serviceGroup.registerService(pDispatcher);
			locator.registerService("dispatcher.block", pDispatcher);

			state.hooks().setBlockRangeConsumerFactory([&dispatcher = *pDispatcher](auto source) {
				return [&dispatcher, source](auto&& range) {
					dispatcher.processElement(ConsumerInput(std::move(range), source));
				};
			});

			state.hooks().setCompletionAwareBlockRangeConsumerFactory([&dispatcher = *pDispatcher](auto source) {
				return [&dispatcher, source](auto&& range, const auto& processingComplete) {
					return dispatcher.processElement(ConsumerInput(std::move(range), source), processingComplete);
				};
			});
		}

		// endregion

		// region transaction

		class TransactionDispatcherBuilder {
		public:
			explicit TransactionDispatcherBuilder(extensions::ServiceState& state)
					: m_state(state)
					, m_nodeConfig(m_state.config().Node)
			{}

		public:
			void addHashConsumers() {
				m_consumers.push_back(CreateTransactionHashCalculatorConsumer(
						m_state.config().Immutable.GenerationHash,
						m_state.pluginManager().transactionRegistry()));
				m_consumers.push_back(CreateTransactionHashCheckConsumer(
						m_state.timeSupplier(),
						extensions::CreateHashCheckOptions(m_nodeConfig.ShortLivedCacheTransactionDuration, m_nodeConfig),
						m_state.hooks().knownHashPredicate(m_state.utCache())));
			}

			std::shared_ptr<ConsumerDispatcher> build(
					const std::shared_ptr<thread::IoThreadPool>& pValidatorPool,
					chain::UtUpdater& utUpdater) {
				m_consumers.push_back(CreateTransactionStatelessValidationConsumer(
						extensions::CreateStatelessValidator(m_state.pluginManager()),
						validators::CreateParallelValidationPolicy(pValidatorPool),
						extensions::SubscriberToSink(m_state.transactionStatusSubscriber())));

				auto disruptorConsumers = DisruptorConsumersFromTransactionConsumers(m_consumers);
				disruptorConsumers.push_back(CreateNewTransactionsConsumer(
						[&utUpdater, newTransactionsSink = m_state.hooks().newTransactionsSink()](auto&& transactionInfos) {
					  // only broadcast transactions that have passed stateful validation on this node
					  auto utUpdateResults = utUpdater.update(transactionInfos);

					  auto filteredTransactionInfos = SelectValid(std::move(transactionInfos), utUpdateResults);
					  if (!filteredTransactionInfos.empty())
						  newTransactionsSink(filteredTransactionInfos);
				}));

				return CreateConsumerDispatcher(
						m_state,
						CreateTransactionConsumerDispatcherOptions(m_nodeConfig),
						std::move(disruptorConsumers));
			}

		private:
			extensions::ServiceState& m_state;
			config::NodeConfiguration m_nodeConfig;
			std::vector<TransactionConsumer> m_consumers;
		};

		void RegisterTransactionDispatcherService(
				const std::shared_ptr<ConsumerDispatcher>& pDispatcher,
				thread::MultiServicePool::ServiceGroup& serviceGroup,
				extensions::ServiceLocator& locator,
				extensions::ServiceState& state) {
			serviceGroup.registerService(pDispatcher);
			locator.registerService("dispatcher.transaction", pDispatcher);

			auto pBatchRangeDispatcher = std::make_shared<extensions::TransactionBatchRangeDispatcher>(*pDispatcher);
			locator.registerRootedService("dispatcher.transaction.batch", pBatchRangeDispatcher);

			state.hooks().setTransactionRangeConsumerFactory([&dispatcher = *pBatchRangeDispatcher](auto source) {
				return [&dispatcher, source](auto&& range) {
					dispatcher.queue(std::move(range), source);
				};
			});

			state.tasks().push_back(extensions::CreateBatchTransactionTask(*pBatchRangeDispatcher, "transaction"));
		}

		// endregion

		chain::UtUpdater& CreateAndRegisterUtUpdater(extensions::ServiceLocator& locator, extensions::ServiceState& state) {
			auto pUtUpdater = std::make_shared<chain::UtUpdater>(
					state.utCache(),
					state.cache(),
					extensions::CreateExecutionConfiguration(state.pluginManager()),
					state.timeSupplier(),
					extensions::SubscriberToSink(state.transactionStatusSubscriber()),
					CreateUtUpdaterThrottle(state));
			locator.registerRootedService("dispatcher.utUpdater", pUtUpdater);

			auto& utUpdater = *pUtUpdater;
			state.hooks().addTransactionsChangeHandler([&utUpdater](const auto& changeInfo) {
				utUpdater.update(changeInfo.AddedTransactionHashes, changeInfo.RevertedTransactionInfos);
			});

			return utUpdater;
		}

		auto CreateAndRegisterRollbackService(
				extensions::ServiceLocator& locator,
				const chain::TimeSupplier& timeSupplier,
				extensions::ServiceState& state) {
			auto pRollbackInfo = std::make_shared<RollbackInfo>(timeSupplier, state);
			locator.registerRootedService("rollbacks", pRollbackInfo);
			return pRollbackInfo;
		}

		void AddRollbackCounter(
				extensions::ServiceLocator& locator,
				const std::string& counterName,
				RollbackResult rollbackResult,
				RollbackCounterType rollbackCounterType) {
			locator.registerServiceCounter<RollbackInfo>("rollbacks", counterName, [rollbackResult, rollbackCounterType](
					const auto& rollbackInfo) {
				return rollbackInfo.view().counter(rollbackResult, rollbackCounterType);
			});
		}

		class DispatcherServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			extensions::ServiceRegistrarInfo info() const override {
				return { "Dispatcher", extensions::ServiceRegistrarPhase::Post_Remote_Peers };
			}

			void registerServiceCounters(extensions::ServiceLocator& locator) override {
				extensions::AddDispatcherCounters(locator, "dispatcher.block", "BLK");
				extensions::AddDispatcherCounters(locator, "dispatcher.transaction", "TX");

				AddRollbackCounter(locator, "RB COMMIT ALL", RollbackResult::Committed, RollbackCounterType::All);
				AddRollbackCounter(locator, "RB COMMIT RCT", RollbackResult::Committed, RollbackCounterType::Recent);
				AddRollbackCounter(locator, "RB IGNORE ALL", RollbackResult::Ignored, RollbackCounterType::All);
				AddRollbackCounter(locator, "RB IGNORE RCT", RollbackResult::Ignored, RollbackCounterType::Recent);
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				// create shared services
				auto pValidatorPool = state.pool().pushIsolatedPool("validator");
				auto& utUpdater = CreateAndRegisterUtUpdater(locator, state);

				// create the block and transaction dispatchers and related services
				// (notice that the dispatcher service group must be after the validator isolated pool in order to allow proper shutdown)
				auto pServiceGroup = state.pool().pushServiceGroup("dispatcher service");

				BlockDispatcherBuilder blockDispatcherBuilder(state);
				blockDispatcherBuilder.addHashConsumers();

				TransactionDispatcherBuilder transactionDispatcherBuilder(state);
				transactionDispatcherBuilder.addHashConsumers();

				auto pRollbackInfo = CreateAndRegisterRollbackService(locator, state.timeSupplier(), state);
				auto pBlockDispatcher = blockDispatcherBuilder.build(pValidatorPool, *pRollbackInfo);
				RegisterBlockDispatcherService(pBlockDispatcher, *pServiceGroup, locator, state);

				auto pTransactionDispatcher = transactionDispatcherBuilder.build(pValidatorPool, utUpdater);
				RegisterTransactionDispatcherService(pTransactionDispatcher, *pServiceGroup, locator, state);
			}
		};
	}

	DECLARE_SERVICE_REGISTRAR(Dispatcher)() {
		return std::make_unique<DispatcherServiceRegistrar>();
	}
}}
