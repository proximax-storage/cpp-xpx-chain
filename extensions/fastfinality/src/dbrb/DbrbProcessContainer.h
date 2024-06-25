/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbProcess.h"
#include "DbrbPacketHandlers.h"
#include "ShardedDbrbProcess.h"

namespace catapult { namespace dbrb {

	class DbrbProcessContainer {
	public:
		DbrbProcessContainer(std::shared_ptr<dbrb::DbrbProcess> pDbrbProcess)
			: m_pDbrbProcess(std::move(pDbrbProcess))
			, m_shardingEnabled(false)
		{}

		DbrbProcessContainer(std::shared_ptr<dbrb::ShardedDbrbProcess> pDbrbProcess)
			: m_pShardedDbrbProcess(std::move(pDbrbProcess))
			, m_shardingEnabled(true)
		{}

	public:
		void shutdown() {
			m_pDbrbProcess.reset();
			m_pShardedDbrbProcess.reset();
		}

	public:
		bool shardingEnabled() const {
			return m_shardingEnabled;
		}

		const auto& currentView() const {
			if (m_shardingEnabled) {
				return m_pShardedDbrbProcess->currentView();
			} else {
				return m_pDbrbProcess->currentView();
			}
		}

		const auto& strand() const {
			if (m_shardingEnabled) {
				return m_pShardedDbrbProcess->strand();
			} else {
				return m_pDbrbProcess->strand();
			}
		}

		auto messageSender() const {
			if (m_shardingEnabled) {
				return m_pShardedDbrbProcess->messageSender();
			} else {
				return m_pDbrbProcess->messageSender();
			}
		}

		const auto& id() const {
			if (m_shardingEnabled) {
				return m_pShardedDbrbProcess->id();
			} else {
				return m_pDbrbProcess->id();
			}
		}

		void registerPacketHandlers(ionet::ServerPacketHandlers& packetHandlers) {
			if (m_shardingEnabled) {
				m_pShardedDbrbProcess->registerPacketHandlers(packetHandlers);
			} else {
				m_pDbrbProcess->registerPacketHandlers(packetHandlers);
			}
		}

		void setValidationCallback(const dbrb::ValidationCallback& callback) {
			if (m_shardingEnabled) {
				m_pShardedDbrbProcess->setValidationCallback(callback);
			} else {
				m_pDbrbProcess->setValidationCallback(callback);
			}
		}

		void setDeliverCallback(const dbrb::DeliverCallback& callback) {
			if (m_shardingEnabled) {
				m_pShardedDbrbProcess->setDeliverCallback(callback);
			} else {
				m_pDbrbProcess->setDeliverCallback(callback);
			}
		}

		bool updateView(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, const Timestamp& now, const Height& height, bool registerSelf) {
			if (m_shardingEnabled) {
				return m_pShardedDbrbProcess->updateView(pConfigHolder, now, height, registerSelf);
			} else {
				return m_pDbrbProcess->updateView(pConfigHolder, now, height, registerSelf);
			}
		}

		void broadcast(const Payload& payload, ViewData recipients) {
			if (m_shardingEnabled) {
				m_pShardedDbrbProcess->broadcast(payload, std::move(recipients));
			} else {
				m_pDbrbProcess->broadcast(payload, std::move(recipients));
			}
		}

		void registerDbrbPushNodesHandler(model::NetworkIdentifier networkIdentifier, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder, ionet::ServerPacketHandlers& packetHandlers) {
			if (m_shardingEnabled) {
				dbrb::RegisterPushNodesHandler(std::weak_ptr<dbrb::ShardedDbrbProcess>(m_pShardedDbrbProcess), networkIdentifier, pConfigHolder, packetHandlers);
			} else {
				dbrb::RegisterPushNodesHandler(std::weak_ptr<dbrb::DbrbProcess>(m_pDbrbProcess), networkIdentifier, pConfigHolder, packetHandlers);
			}
		}

		void registerDbrbPullNodesHandler(ionet::ServerPacketHandlers& packetHandlers) {
			if (m_shardingEnabled) {
				dbrb::RegisterPullNodesHandler(std::weak_ptr<dbrb::ShardedDbrbProcess>(m_pShardedDbrbProcess), packetHandlers);
			} else {
				dbrb::RegisterPullNodesHandler(std::weak_ptr<dbrb::DbrbProcess>(m_pDbrbProcess), packetHandlers);
			}
		}

		void registerDbrbRemoveNodeRequestHandler(const crypto::KeyPair& keyPair, ionet::ServerPacketHandlers& packetHandlers) {
			if (m_shardingEnabled) {
				dbrb::RegisterRemoveNodeRequestHandler(std::weak_ptr<dbrb::ShardedDbrbProcess>(m_pShardedDbrbProcess), keyPair, packetHandlers);
			} else {
				dbrb::RegisterRemoveNodeRequestHandler(std::weak_ptr<dbrb::DbrbProcess>(m_pDbrbProcess), keyPair, packetHandlers);
			}
		}

		void registerDbrbRemoveNodeResponseHandler(ionet::ServerPacketHandlers& packetHandlers) {
			if (m_shardingEnabled) {
				dbrb::RegisterRemoveNodeResponseHandler(std::weak_ptr<dbrb::ShardedDbrbProcess>(m_pShardedDbrbProcess), packetHandlers);
			} else {
				dbrb::RegisterRemoveNodeResponseHandler(std::weak_ptr<dbrb::DbrbProcess>(m_pDbrbProcess), packetHandlers);
			}
		}

	private:
		std::shared_ptr<dbrb::DbrbProcess> m_pDbrbProcess;
		std::shared_ptr<dbrb::ShardedDbrbProcess> m_pShardedDbrbProcess;
		bool m_shardingEnabled;
	};
}}