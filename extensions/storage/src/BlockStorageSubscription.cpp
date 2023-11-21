/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/notification_handlers/HandlerNotificationSubscriber.h"
#include "catapult/notification_handlers/HandlerContext.h"
#include "BlockStorageSubscription.h"

namespace catapult { namespace storage {

	namespace {
		model::WeakEntityInfos ExtractEntityInfos(const model::BlockElement& element) {
			model::WeakEntityInfos entityInfos;
			model::ExtractEntityInfos(element, entityInfos);
			return entityInfos;
		}

		class BlockStorageSubscription final : public io::BlockChangeSubscriber {
		public:
			explicit BlockStorageSubscription(extensions::ProcessBootstrapper& bootstrapper, HandlerPointer pHandler)
				: m_pluginManager(bootstrapper.pluginManager())
				, m_cacheHolder(bootstrapper.cacheHolder())
				, m_pConfigHolder(bootstrapper.configHolder())
				, m_pHandler(std::move(pHandler))
			{}

		public:
			void notifyBlock(const model::BlockElement& blockElement) override {
				// We load extensions before plugins, so we can't init publisher in constructor
				if (!m_pNotificationPublisher) {
					m_pNotificationPublisher = m_pluginManager.createNotificationPublisher();
				}
				const auto& config = m_pConfigHolder->Config(blockElement.Block.Height);
				auto cacheView = m_cacheHolder.cache().createView();
				auto readCache = cacheView.toReadOnly();

				auto handlerContext = notification_handlers::HandlerContext(
					config,
					blockElement.Block.Height,
					blockElement.Block.Timestamp,
					m_pluginManager.createResolverContext(readCache),
					readCache
				);
				notification_handlers::HandlerNotificationSubscriber handleSubscriber(*m_pHandler, handlerContext);
				for (const auto& entity : ExtractEntityInfos(blockElement)) {
					m_pNotificationPublisher->publish(entity, handleSubscriber);
				}
			}

			void notifyDropBlocksAfter(Height height) override {
				// TODO: Do we need to support revert of blocks?
			}

		private:
			plugins::PluginManager& m_pluginManager;
			extensions::CacheHolder& m_cacheHolder;
			std::shared_ptr<config::BlockchainConfigurationHolder> m_pConfigHolder;
			HandlerPointer m_pHandler;
			std::unique_ptr<model::NotificationPublisher> m_pNotificationPublisher;
		};
	}

	std::unique_ptr<io::BlockChangeSubscriber> CreateBlockStorageSubscription(
			extensions::ProcessBootstrapper& bootstrapper, const HandlerPointer& pHandler) {
		return std::make_unique<BlockStorageSubscription>(bootstrapper, pHandler);
	}
}}
