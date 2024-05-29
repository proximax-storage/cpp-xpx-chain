/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "FastFinalityShutdownService.h"
#include "fastfinality/src/dbrb/TransactionSender.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/thread/MultiServicePool.h"
#include "catapult/utils/NetworkTime.h"
#include <utility>

namespace catapult { namespace fastfinality {

	namespace {
		class FastFinalityShutdownServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			explicit FastFinalityShutdownServiceRegistrar(std::shared_ptr<dbrb::TransactionSender> pTransactionSender)
				: m_pTransactionSender(std::move(pTransactionSender))
			{}

			class DbrbNodeRemover {
			public:
				explicit DbrbNodeRemover(std::shared_ptr<dbrb::TransactionSender> pTransactionSender, const plugins::PluginManager& pluginManager, const dbrb::ProcessId& id)
					: m_pTransactionSender(std::move(pTransactionSender))
					, m_pluginManager(pluginManager)
					, m_id(id)
				{}

			public:
				void shutdown() {
					if (m_pluginManager.config().EnableRemovingDbrbProcessOnShutdown) {
						m_pTransactionSender->disableAddDbrbProcessTransaction();
						auto view = m_pluginManager.dbrbViewFetcher().getView(utils::NetworkTime());
						if (m_pTransactionSender->isAddDbrbProcessTransactionSent() || view.find(m_id) != view.cend()) {
							auto future = m_pTransactionSender->sendRemoveDbrbProcessTransaction();
							future.wait_for(std::chrono::minutes(1));
						}
					}
				}

			private:
				std::shared_ptr<dbrb::TransactionSender> m_pTransactionSender;
				const plugins::PluginManager& m_pluginManager;
				dbrb::ProcessId m_id;
			};

			extensions::ServiceRegistrarInfo info() const override {
				return { "FastFinality", extensions::ServiceRegistrarPhase::Shutdown_Handlers };
			}

			void registerServiceCounters(extensions::ServiceLocator& locator) override {
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				auto pServiceGroup = state.pool().pushServiceGroup("DBRB node remover");
				pServiceGroup->registerService(std::make_shared<DbrbNodeRemover>(m_pTransactionSender, state.pluginManager(), locator.keyPair().publicKey()));
			}

		private:
			std::shared_ptr<dbrb::TransactionSender> m_pTransactionSender;
		};
	}

	DECLARE_SERVICE_REGISTRAR(FastFinalityShutdown)(std::shared_ptr<dbrb::TransactionSender> pTransactionSender) {
		return std::make_unique<FastFinalityShutdownServiceRegistrar>(std::move(pTransactionSender));
	}
}}
