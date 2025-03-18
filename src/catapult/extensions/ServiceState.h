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

#pragma once
#include "ServerHooks.h"
#include "catapult/config/BlockchainConfiguration.h"
#include "catapult/io/BlockChangeSubscriber.h"
#include "catapult/ionet/PacketHandlers.h"
#include "catapult/net/PacketIoPickerContainer.h"
#include "catapult/plugins/PluginManager.h"
#include "catapult/thread/Task.h"
#include "catapult/notification_handlers/NotificationHandlerTypes.h"

namespace catapult {
	namespace cache {
		class CatapultCache;
		class MemoryUtCacheProxy;
	}
	namespace extensions { class LocalNodeChainScore; }
	namespace io { class BlockStorageCache; }
	namespace ionet { class NodeContainer; }
	namespace state { struct CatapultState; }
	namespace subscribers {
		class NodeSubscriber;
		class StateChangeSubscriber;
		class TransactionStatusSubscriber;
		class BlockChangeSubscriber;
	}
	namespace thread { class MultiServicePool; }
	namespace utils { class DiagnosticCounter; }
}

namespace catapult { namespace extensions {

	/// State that is used as part of service registration.
	class ServiceState {
	public:
		/// Creates service state around \a config, \a nodes, \a cache, \a state, \a storage, \a score, \a utCache, \a timeSupplier
		/// \a transactionStatusSubscriber, \a stateChangeSubscriber, \a nodeSubscriber, \a counters, \a pluginManager and \a pool.
		ServiceState(
				ionet::NodeContainer& nodes,
				cache::CatapultCache& cache,
				state::CatapultState& state,
				io::BlockStorageCache& storage,
				LocalNodeChainScore& score,
				cache::MemoryUtCacheProxy& utCache,
				const supplier<Timestamp>& timeSupplier,
				subscribers::TransactionStatusSubscriber& transactionStatusSubscriber,
				subscribers::StateChangeSubscriber& stateChangeSubscriber,
				subscribers::NodeSubscriber& nodeSubscriber,
				io::BlockChangeSubscriber& postBlockCommitSubscriber,
				const notification_handlers::AggregateNotificationHandler& notificationSubscriber,
				const std::vector<utils::DiagnosticCounter>& counters,
				const plugins::PluginManager& pluginManager,
				thread::MultiServicePool& pool)
				: m_nodes(nodes)
				, m_cache(cache)
				, m_state(state)
				, m_storage(storage)
				, m_score(score)
				, m_utCache(utCache)
				, m_timeSupplier(timeSupplier)
				, m_transactionStatusSubscriber(transactionStatusSubscriber)
				, m_stateChangeSubscriber(stateChangeSubscriber)
				, m_nodeSubscriber(nodeSubscriber)
				, m_postBlockCommitSubscriber(postBlockCommitSubscriber)
				, m_notificationSubscriber(notificationSubscriber)
				, m_counters(counters)
				, m_pluginManager(pluginManager)
				, m_pool(pool)
				, m_packetHandlers(m_pluginManager.configHolder()->Config().Node.MaxPacketDataSize.bytes32())
				, m_maxChainHeight(Height(0))
		{}

	public:
		/// Gets the config.
		const auto& config(const Height& height) const {
			return m_pluginManager.configHolder()->Config(height);
		}

		/// Gets the config.
		const auto& config() const {
			return m_pluginManager.configHolder()->Config();
		}

		/// Gets the nodes.
		auto& nodes() const {
			return m_nodes;
		}

		/// Gets the cache.
		auto& cache() const {
			return m_cache;
		}

		/// Gets the state.
		auto& state() const {
			return m_state;
		}

		/// Gets the storage.
		auto& storage() const {
			return m_storage;
		}

		/// Gets the score.
		auto& score() const {
			return m_score;
		}

		/// Gets the unconfirmed transactions cache.
		auto& utCache() const {
			return m_utCache;
		}

		/// Gets the time supplier.
		auto timeSupplier() const {
			return m_timeSupplier;
		}

		/// Gets the transaction status subscriber.
		auto& transactionStatusSubscriber() const {
			return m_transactionStatusSubscriber;
		}

		/// Gets the state change subscriber.
		auto& stateChangeSubscriber() const {
			return m_stateChangeSubscriber;
		}

		/// Gets the node subscriber.
		auto& nodeSubscriber() const {
			return m_nodeSubscriber;
		}

		/// Gets the post block commit subscriber.
		auto& postBlockCommitSubscriber() const {
			return m_postBlockCommitSubscriber;
		}

		/// Gets the notification subscriber.
		const auto& notificationSubscriber() const {
			return m_notificationSubscriber;
		}

		/// Gets the (basic) counters.
		/// \note These counters are node counters and do not include counters registered via ServiceLocator.
		const auto& counters() const {
			return m_counters;
		}

		/// Gets the plugin manager.
		const auto& pluginManager() const {
			return m_pluginManager;
		}

		/// Gets the multiservice pool.
		auto& pool() {
			return m_pool;
		}

		/// Gets the tasks.
		auto& tasks() {;
			return m_tasks;
		}

		/// Gets the packet handlers.
		auto& packetHandlers() {
			return m_packetHandlers;
		}

		/// Gets the server hooks.
		const auto& hooks() const {
			return m_hooks;
		}

		/// Gets the server hooks.
		auto& hooks() {
			return m_hooks;
		}

		/// Gets the packet io pickers.
		auto& packetIoPickers() {
			return m_packetIoPickers;
		}

		/// Gets the network identifier.
		auto networkIdentifier() const {
			return config().Immutable.NetworkIdentifier;
		}

		const Height& maxChainHeight() const {
			return m_maxChainHeight;
		}

		void setMaxChainHeight(const Height& height) {
			m_maxChainHeight = height;
		}

	private:
		// references
		ionet::NodeContainer& m_nodes;
		cache::CatapultCache& m_cache;
		state::CatapultState& m_state;
		io::BlockStorageCache& m_storage;
		LocalNodeChainScore& m_score;
		cache::MemoryUtCacheProxy& m_utCache;
		supplier<Timestamp> m_timeSupplier;

		subscribers::TransactionStatusSubscriber& m_transactionStatusSubscriber;
		subscribers::StateChangeSubscriber& m_stateChangeSubscriber;
		subscribers::NodeSubscriber& m_nodeSubscriber;
		io::BlockChangeSubscriber& m_postBlockCommitSubscriber;
		const notification_handlers::AggregateNotificationHandler& m_notificationSubscriber;

		const std::vector<utils::DiagnosticCounter>& m_counters;
		const plugins::PluginManager& m_pluginManager;
		thread::MultiServicePool& m_pool;

		// owned
		std::vector<thread::Task> m_tasks;
		ionet::ServerPacketHandlers m_packetHandlers;
		ServerHooks m_hooks;
		net::PacketIoPickerContainer m_packetIoPickers;
		Height m_maxChainHeight;
	};
}}
