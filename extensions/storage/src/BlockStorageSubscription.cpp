/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/validators/ValidatingNotificationSubscriber.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/plugins/PluginManager.h"
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
			explicit BlockStorageSubscription(extensions::ProcessBootstrapper& bootstrapper, ValidatorPointer pValidator)
				: m_pluginManager(bootstrapper.pluginManager())
				, m_cacheHolder(bootstrapper.cacheHolder())
				, m_configHolder(bootstrapper.configHolder())
				, m_pValidator(pValidator)
			{}

		public:
			void notifyBlock(const model::BlockElement& blockElement) override {
				// We load extensions before plugins, so we can't init publisher in constructor
				if (!m_notificationPublisher) {
					m_notificationPublisher = m_pluginManager.createNotificationPublisher();
				}
				const auto& config = m_configHolder->Config(blockElement.Block.Height);
				auto cacheView = m_cacheHolder.cache().createView();
				auto readCache = cacheView.toReadOnly();

				auto validatorContext = validators::ValidatorContext(
					config,
					blockElement.Block.Height,
					blockElement.Block.Timestamp,
					m_pluginManager.createResolverContext(readCache),
					readCache
				);
				validators::ValidatingNotificationSubscriber validatingSubscriber(*m_pValidator, validatorContext);
				for (const auto& entity : ExtractEntityInfos(blockElement)) {
					m_notificationPublisher->publish(entity, validatingSubscriber);
				}
			}

			void notifyDropBlocksAfter(Height height) override {
				// TODO: Do we need to support revert of blocks?
			}

		private:
			plugins::PluginManager& m_pluginManager;
			extensions::CacheHolder& m_cacheHolder;
			std::shared_ptr<config::BlockchainConfigurationHolder> m_configHolder;
			ValidatorPointer m_pValidator;
			std::unique_ptr<model::NotificationPublisher> m_notificationPublisher;
		};
	}

	std::unique_ptr<io::BlockChangeSubscriber> CreateBlockStorageSubscription(
			extensions::ProcessBootstrapper& bootstrapper, ValidatorPointer pValidator) {
		return std::make_unique<BlockStorageSubscription>(bootstrapper, pValidator);
	}
}}
