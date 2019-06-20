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

#include "PtDispatcherService.h"
#include "PtBootstrapperService.h"
#include "PtUtils.h"
#include "partialtransaction/src/chain/PtUpdater.h"
#include "partialtransaction/src/chain/PtValidator.h"
#include "partialtransaction/src/handlers/CosignatureHandler.h"
#include "partialtransaction/src/handlers/PtHandlers.h"
#include "catapult/cache_tx/MemoryPtCache.h"
#include "catapult/consumers/RecentHashCache.h"
#include "catapult/consumers/ReclaimMemoryInspector.h"
#include "catapult/consumers/TransactionConsumers.h"
#include "catapult/disruptor/ConsumerDispatcher.h"
#include "catapult/extensions/DispatcherUtils.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/extensions/ServiceUtils.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/thread/MultiServicePool.h"

using namespace catapult::consumers;
using namespace catapult::disruptor;
using namespace catapult::extensions;

namespace catapult { namespace partialtransaction {

	namespace {
		constexpr auto Service_Name = "pt.dispatcher";
		constexpr auto Api_Partial_Service_Name = "api.partial";

		using CosignaturesSink = consumer<const std::vector<model::DetachedCosignature>&>;

		Hash256 ToHash(const model::DetachedCosignature& cosignature) {
			// the R-part of the signature is good enough for a hash
			Hash256 hash;
			std::memcpy(hash.data(), cosignature.Signature.data(), Hash256_Size);
			return hash;
		}

		ConsumerDispatcherOptions CreateTransactionConsumerDispatcherOptions(const config::NodeConfiguration& config) {
			auto options = ConsumerDispatcherOptions("partial transaction dispatcher", config.TransactionDisruptorSize);
			options.ElementTraceInterval = config.TransactionElementTraceInterval;
			options.ShouldThrowWhenFull = config.ShouldAbortWhenDispatcherIsFull;
			return options;
		}

		std::unique_ptr<ConsumerDispatcher> CreateConsumerDispatcher(
				const ConsumerDispatcherOptions& options,
				const std::vector<DisruptorConsumer>& consumers) {
			auto reclaimMemoryInspector = CreateReclaimMemoryInspector();
			return std::make_unique<ConsumerDispatcher>(options, consumers, reclaimMemoryInspector);
		}

		auto CreateKnownHashPredicate(const cache::MemoryPtCacheProxy& ptCache, ServiceState& state) {
			auto knownHashPredicate = state.hooks().knownHashPredicate(state.utCache());
			return [&ptCache, knownHashPredicate](auto timestamp, const auto& hash) {
				return ptCache.view().find(hash) || knownHashPredicate(timestamp, hash);
			};
		}

		auto CreateNewTransactionSink(const extensions::ServiceLocator& locator) {
			return extensions::CreatePushEntitySink<extensions::SharedNewTransactionsSink>(
					locator,
					Api_Partial_Service_Name,
					ionet::PacketType::Push_Partial_Transactions);
		}

		auto CreateNewCosignaturesSink(const extensions::ServiceLocator& locator) {
			return extensions::CreatePushEntitySink<CosignaturesSink>(locator, Api_Partial_Service_Name);
		}

		class TransactionDispatcherBuilder {
		public:
			explicit TransactionDispatcherBuilder(extensions::ServiceState& state)
					: m_state(state)
					, m_nodeConfig(m_state.config().Node)
			{}

		public:
			void addHashConsumers(const cache::MemoryPtCacheProxy& ptCache) {
				m_consumers.push_back(CreateTransactionHashCalculatorConsumer(
						m_state.config().BlockChain.Network.GenerationHash,
						m_state.pluginManager().transactionRegistry()));
				m_consumers.push_back(CreateTransactionHashCheckConsumer(
						m_state.timeSupplier(),
						extensions::CreateHashCheckOptions(m_nodeConfig.ShortLivedCacheTransactionDuration, m_nodeConfig),
						CreateKnownHashPredicate(ptCache, m_state)));
			}

			std::shared_ptr<ConsumerDispatcher> build(
					chain::PtUpdater& ptUpdater,
					const extensions::SharedNewTransactionsSink& newTransactionSink) {
				auto disruptorConsumers = DisruptorConsumersFromTransactionConsumers(m_consumers);
				disruptorConsumers.push_back(CreateNewTransactionsConsumer([&ptUpdater, newTransactionSink](auto&& transactionInfos) {
					newTransactionSink(transactionInfos);
					std::vector<thread::future<chain::TransactionUpdateResult>> futures;
					for (const auto& transactionInfo : transactionInfos)
						futures.push_back(ptUpdater.update(transactionInfo));

					thread::when_all(std::move(futures)).get();
				}));

				return CreateConsumerDispatcher(CreateTransactionConsumerDispatcherOptions(m_nodeConfig), disruptorConsumers);
			}

		private:
			extensions::ServiceState& m_state;
			const config::NodeConfiguration& m_nodeConfig;
			std::vector<TransactionConsumer> m_consumers;
		};

		// 1. Pt updater gets updater pool, registering updater as a rooted service would cause deadlock during shutdown.
		// 2. Dispatcher needs to extend lifetime of pt updater
		// 3. usual shared_ptr-based tying of updater to dispatcher won't work, cause that would result in following order:
		//    a. dispatcher is shutdown - but not freed
		//    b. updater pool is shutdown - but dispatcher holds updater, so we'd have a loop
		//
		// That's why we need additional wrapper, to tie updater to dispatcher and enforce proper shutdown
		class DispatcherServiceRegistrar {
		public:
			DispatcherServiceRegistrar(
					const std::shared_ptr<ConsumerDispatcher>& pDispatcher,
					std::unique_ptr<chain::PtUpdater>&& pUpdater)
					: m_pDispatcher(pDispatcher)
					, m_pUpdater(std::move(pUpdater))
			{}

		public:
			void shutdown() {
				m_pDispatcher->shutdown();
				m_pUpdater.reset();
			}

		private:
			std::shared_ptr<ConsumerDispatcher> m_pDispatcher;
			std::unique_ptr<chain::PtUpdater> m_pUpdater;
		};

		void RegisterTransactionDispatcherService(
				const std::shared_ptr<ConsumerDispatcher>& pDispatcher,
				chain::PtUpdater& ptUpdater,
				extensions::ServiceLocator& locator,
				extensions::ServiceState& state) {
			locator.registerService(Service_Name, pDispatcher);

			auto pBatchRangeDispatcher = std::make_shared<extensions::TransactionBatchRangeDispatcher>(*pDispatcher);
			locator.registerRootedService("pt.dispatcher.batch", pBatchRangeDispatcher);

			// register hooks
			const auto& nodeConfig = state.config().Node;
			auto pRecentHashCache = std::make_shared<RecentHashCache>(
					state.timeSupplier(),
					extensions::CreateHashCheckOptions(nodeConfig.ShortLivedCacheTransactionDuration, nodeConfig));
			auto cosignaturesSink = CreateNewCosignaturesSink(locator);
			auto pCacheLock = std::make_shared<utils::SpinLock>();
			auto& hooks = GetPtServerHooks(locator);
			hooks.setCosignedTransactionInfosConsumer(
					[&dispatcher = *pBatchRangeDispatcher, &ptUpdater, pRecentHashCache, cosignaturesSink, pCacheLock](auto&& infos) {
				CATAPULT_LOG(debug) << "pushing infos " << infos.size() << " to pt dispatcher";
				std::vector<model::DetachedCosignature> newCosignatures;
				SplitCosignedTransactionInfos(
						std::move(infos),
						[&dispatcher](auto&& transactionRange) {
							dispatcher.queue(std::move(transactionRange), InputSource::Remote_Pull);
						},
						[&ptUpdater, &newCosignatures, pRecentHashCache, pCacheLock](auto&& cosignature) {
							utils::SpinLockGuard guard(*pCacheLock);
							if (pRecentHashCache->add(ToHash(cosignature))) {
								ptUpdater.update(cosignature);
								newCosignatures.push_back(cosignature);
							}
						});

				if (!newCosignatures.empty())
					cosignaturesSink(newCosignatures);
			});

			hooks.setPtRangeConsumer([&dispatcher = *pBatchRangeDispatcher](auto&& transactionRange) {
				dispatcher.queue(std::move(transactionRange), InputSource::Remote_Push);
			});

			hooks.setCosignatureRangeConsumer([&ptUpdater, pRecentHashCache, cosignaturesSink, pCacheLock](auto&& cosignatureRange) {
				utils::SpinLockGuard guard(*pCacheLock);
				std::vector<model::DetachedCosignature> newCosignatures;
				for (const auto& cosignature : cosignatureRange.Range) {
					if (pRecentHashCache->add(ToHash(cosignature))) {
						ptUpdater.update(cosignature);
						newCosignatures.push_back(cosignature);
					}
				}

				if (!newCosignatures.empty())
					cosignaturesSink(newCosignatures);
			});

			state.tasks().push_back(extensions::CreateBatchTransactionTask(*pBatchRangeDispatcher, "partial transaction"));
		}

		std::unique_ptr<chain::PtUpdater> CreateAndRegisterPtUpdater(cache::MemoryPtCacheProxy& ptCache, extensions::ServiceState& state) {
			auto pUpdaterPool = state.pool().pushIsolatedPool("ptUpdater");

			// validator needs to be created here because bootstrapper does not have cache nor all validators registered
			auto pValidator = chain::CreatePtValidator(state.cache(), state.timeSupplier(), state.pluginManager());

			auto transactionRangeConsumerFactory = state.hooks().transactionRangeConsumerFactory();
			return std::make_unique<chain::PtUpdater>(
					ptCache,
					std::move(pValidator),
					[transactionRangeConsumerFactory](auto&& pTransaction) {
						auto consumer = transactionRangeConsumerFactory(disruptor::InputSource::Local);
						consumer(model::TransactionRange::FromEntity(std::move(pTransaction)));
					},
					extensions::SubscriberToSink(state.transactionStatusSubscriber()),
					pUpdaterPool);
		}

		class PtDispatcherServiceRegistrar : public ServiceRegistrar {
		public:
			ServiceRegistrarInfo info() const override {
				return { "PtDispatcher", ServiceRegistrarPhase::Post_Range_Consumers };
			}

			void registerServiceCounters(ServiceLocator& locator) override {
				extensions::AddDispatcherCounters(locator, Service_Name, "PT");
			}

			void registerServices(ServiceLocator& locator, ServiceState& state) override {
				// partial transaction updater
				auto& ptCache = GetMemoryPtCache(locator);
				auto pPtUpdater = CreateAndRegisterPtUpdater(ptCache, state);

				// partial transaction dispatcher
				auto pServiceGroup = state.pool().pushServiceGroup("partial dispatcher");
				TransactionDispatcherBuilder dispatcherBuilder(state);
				dispatcherBuilder.addHashConsumers(ptCache);

				auto pDispatcher = dispatcherBuilder.build(*pPtUpdater, CreateNewTransactionSink(locator));
				RegisterTransactionDispatcherService(pDispatcher, *pPtUpdater, locator, state);

				// extend the lifetimes of pDispatcher and pPtUpdater
				pServiceGroup->registerService(std::make_shared<DispatcherServiceRegistrar>(pDispatcher, std::move(pPtUpdater)));
			}
		};
	}

	std::unique_ptr<ServiceRegistrar> CreatePtDispatcherServiceRegistrar() {
		return std::make_unique<PtDispatcherServiceRegistrar>();
	}
}}
