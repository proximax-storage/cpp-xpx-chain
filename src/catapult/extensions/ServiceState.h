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
#include "catapult/config/LocalNodeConfiguration.h"
#include "catapult/ionet/PacketHandlers.h"
#include "catapult/net/PacketIoPickerContainer.h"
#include "catapult/thread/Task.h"
#include "LocalNodeStateRef.h"
#include "ServerHooks.h"
#include "ServiceState.h"

namespace catapult {
	namespace cache {
		class CatapultCache;
		class MemoryUtCacheProxy;
	}
	namespace io { class BlockStorageCache; }
	namespace ionet { class NodeContainer; }
	namespace plugins { class PluginManager; }
	namespace state { struct CatapultState; }
	namespace subscribers {
		class NodeSubscriber;
		class StateChangeSubscriber;
		class TransactionStatusSubscriber;
	}
	namespace thread { class MultiServicePool; }
	namespace utils { class DiagnosticCounter; }
}

namespace catapult { namespace extensions {

	/// State that is used as part of service registration.
	class ServiceState {
	public:
		/// Creates service state around \a config, \a nodes, \a cache, \a state, \a storage, \a utCache, \a timeSupplier
		/// \a transactionStatusSubscriber, \a stateChangeSubscriber, \a nodeSubscriber, \a counters, \a pluginManager and \a pool.
		ServiceState(
				extensions::LocalNodeStateRef& state,
				ionet::NodeContainer& nodes,
				cache::MemoryUtCacheProxy& utCache,
				const supplier<Timestamp>& timeSupplier,
				subscribers::TransactionStatusSubscriber& transactionStatusSubscriber,
				subscribers::StateChangeSubscriber& stateChangeSubscriber,
				subscribers::NodeSubscriber& nodeSubscriber,
				const std::vector<utils::DiagnosticCounter>& counters,
				const plugins::PluginManager& pluginManager,
				thread::MultiServicePool& pool)
				: m_state(state)
				, m_nodes(nodes)
				, m_utCache(utCache)
				, m_timeSupplier(timeSupplier)
				, m_transactionStatusSubscriber(transactionStatusSubscriber)
				, m_stateChangeSubscriber(stateChangeSubscriber)
				, m_nodeSubscriber(nodeSubscriber)
				, m_counters(counters)
				, m_pluginManager(pluginManager)
				, m_pool(pool)
				, m_packetHandlers(m_state.Config.Node.MaxPacketDataSize.bytes32())
		{}

	public:
		/// Gets the nodes.
		auto& nodeLocalState() const {
			return m_state;
		}

		/// Gets the config.
		const auto& config() const {
			return m_state.Config;
		}

		/// Gets the nodes.
		auto& nodes() const {
			return m_nodes;
		}

		/// Gets the current cache.
		auto& currentCache() const {
			return m_state.CurrentCache;
		}

		/// Gets the previous cache.
		auto& previousCache() const {
			return m_state.PreviousCache;
		}

		/// Gets the state.
		auto& state() const {
			return m_state.State;
		}

		/// Gets the storage.
		auto& storage() const {
			return m_state.Storage;
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

	private:
		// references
		extensions::LocalNodeStateRef& m_state;
		ionet::NodeContainer& m_nodes;
		cache::MemoryUtCacheProxy& m_utCache;
		supplier<Timestamp> m_timeSupplier;

		subscribers::TransactionStatusSubscriber& m_transactionStatusSubscriber;
		subscribers::StateChangeSubscriber& m_stateChangeSubscriber;
		subscribers::NodeSubscriber& m_nodeSubscriber;

		const std::vector<utils::DiagnosticCounter>& m_counters;
		const plugins::PluginManager& m_pluginManager;
		thread::MultiServicePool& m_pool;

		// owned
		std::vector<thread::Task> m_tasks;
		ionet::ServerPacketHandlers m_packetHandlers;
		ServerHooks m_hooks;
		net::PacketIoPickerContainer m_packetIoPickers;
	};
}}
