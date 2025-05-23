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

#include "PeersConnectionTasks.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "catapult/cache_core/ImportanceView.h"
#include "catapult/extensions/ServiceState.h"
#include "catapult/net/PacketWriters.h"

namespace catapult { namespace extensions {

	// region CreateNodeAger

	NodeAger CreateNodeAger(
			ionet::ServiceIdentifier serviceId,
			const config::NodeConfiguration::ConnectionsSubConfiguration& config,
			ionet::NodeContainer& nodes) {
		return [&nodes, serviceId, config](const auto& identities) {
			// age all connections
			auto modifier = nodes.modifier();
			modifier.ageConnections(serviceId, identities);
			modifier.ageConnectionBans(serviceId, config.MaxConnectionBanAge, config.NumConsecutiveFailuresBeforeBanning);
		};
	}

	// endregion

	// region SelectorSettings

	SelectorSettings::SelectorSettings(
			extensions::ServiceState& state,
			ionet::ServiceIdentifier serviceId,
			ionet::NodeRoles requiredRole)
			: Nodes(state.nodes())
			, ServiceId(serviceId)
			, RequiredRole(requiredRole)
			, Config(state.config().Node.OutgoingConnections)
			, ImportanceRetriever([&state](const auto& publicKey) {
				auto cacheView = state.cache().createView();
				const auto& accountStateCache = cacheView.sub<cache::AccountStateCache>();
				cache::ReadOnlyAccountStateCache readOnlyAccountStateCache(accountStateCache);
				cache::ImportanceView importanceView(readOnlyAccountStateCache);
				return ImportanceDescriptor{
					importanceView.getAccountImportanceOrDefault(publicKey, cacheView.height()),
					state.config().Network.TotalChainImportance
				};
			})
	{}

	SelectorSettings::SelectorSettings(
			extensions::ServiceState& state,
			ionet::ServiceIdentifier serviceId)
			: SelectorSettings(state, serviceId, ionet::NodeRoles::None)
	{}

	// endregion

	// region CreateNodeSelector / CreateConnectPeersTask

	namespace {
		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			switch (connectCode) {
			case net::PeerConnectCode::Accepted:
				return utils::LogLevel::Info;

			default:
				return utils::LogLevel::Warning;
			}
		}

		class AddCandidateProcessor {
		private:
			using ConnectResult = std::pair<Key, net::PeerConnectCode>;
			using ConnectResultsFuture = thread::future<std::vector<thread::future<ConnectResult>>>;

		public:
			struct AddState {
				ionet::NodeContainer& Nodes;
				net::PacketWriters& PacketWriters;
				ionet::ServiceIdentifier ServiceId;
			};

		public:
			AddCandidateProcessor(const AddState& state) : m_state(state)
			{}

		public:
			thread::future<thread::TaskResult> process(const ionet::NodeSet& addCandidates) {
				if (addCandidates.empty()) {
					CATAPULT_LOG(debug) << "no add candidates for service " << m_state.ServiceId;
					return thread::make_ready_future(thread::TaskResult::Continue);
				}

				auto connectFutures = createConnectFutures(addCandidates);
				return thread::when_all(std::move(connectFutures)).then([state = m_state](auto&& connectResultsFuture) {
					// update interaction information for all nodes
					UpdateInteractions(state, std::move(connectResultsFuture));
					return thread::TaskResult::Continue;
				});
			}

		private:
			std::vector<thread::future<ConnectResult>> createConnectFutures(const ionet::NodeSet& addCandidates) {
				auto i = 0u;
				std::vector<thread::future<ConnectResult>> futures(addCandidates.size());
				for (const auto& node : addCandidates) {
					auto pPromise = std::make_shared<thread::promise<ConnectResult>>();
					futures[i++] = pPromise->get_future();

					m_state.PacketWriters.connect(node, [node, pPromise](const auto& connectResult) {
						CATAPULT_LOG_LEVEL(MapToLogLevel(connectResult.Code))
								<< "connection attempt to " << node << " completed with " << connectResult.Code;
						pPromise->set_value(std::make_pair(node.identityKey(), connectResult.Code));
					});
				}

				return futures;
			}

		private:
			static void UpdateInteractions(const AddState& state, ConnectResultsFuture&& connectResultsFuture) {
				auto modifier = state.Nodes.modifier();
				for (auto& resultFuture : connectResultsFuture.get()) {
					auto connectResult = resultFuture.get();
					auto& connectionState = modifier.provisionConnectionState(state.ServiceId, connectResult.first);
					if (net::PeerConnectCode::Accepted == connectResult.second) {
						modifier.incrementSuccesses(connectResult.first);
						connectionState.NumConsecutiveFailures = 0;
					} else {
						modifier.incrementFailures(connectResult.first);
						++connectionState.NumConsecutiveFailures;
					}
				}

				if (0 == state.PacketWriters.numActiveWriters())
					CATAPULT_LOG(warning) << "unable to connect to any nodes for service " << state.ServiceId;
			}

		private:
			AddState m_state;
		};
	}

	NodeSelector CreateNodeSelector(const SelectorSettings& settings) {
		// 1. provision all existing nodes with a supported role
		settings.Nodes.modifier().addConnectionStates(settings.ServiceId, settings.RequiredRole);

		// 2. create a selector around the nodes and configuration
		extensions::NodeSelectionConfiguration selectionConfig{
			settings.ServiceId,
			settings.RequiredRole,
			settings.Config.MaxConnections,
			settings.Config.MaxConnectionAge
		};
		return [&nodes = settings.Nodes, importanceRetriever = settings.ImportanceRetriever, selectionConfig]() {
			return SelectNodes(nodes, selectionConfig, importanceRetriever);
		};
	}

	thread::Task CreateConnectPeersTask(const SelectorSettings& settings, net::PacketWriters& packetWriters) {
		auto selector = CreateNodeSelector(settings);
		return CreateConnectPeersTask(settings, packetWriters, selector);
	}

	thread::Task CreateConnectPeersTask(
			const SelectorSettings& settings,
			net::PacketWriters& packetWriters,
			const NodeSelector& selector) {
		auto serviceId = settings.ServiceId;
		auto ager = CreateNodeAger(serviceId, settings.Config, settings.Nodes);
		return thread::CreateNamedTask("connect peers task", [serviceId, ager, selector, &nodes = settings.Nodes, &packetWriters]() {
			// 1. age all connections
			ager(packetWriters.identities());

			// 2. select add and remove candidates
			auto result = selector();

			// 3. process remove candidates
			for (const auto& key : result.RemoveCandidates)
				packetWriters.closeOne(key);

			// 4. process add candidates
			AddCandidateProcessor processor({ nodes, packetWriters, serviceId });
			return processor.process(result.AddCandidates);
		});
	}

	// endregion

	// region CreateRemoveOnlyNodeSelector / CreateAgePeersTask

	RemoveOnlyNodeSelector CreateRemoveOnlyNodeSelector(const SelectorSettings& settings) {
		// create a selector around the nodes and configuration
		extensions::NodeAgingConfiguration selectionConfig{
			settings.ServiceId,
			settings.Config.MaxConnections,
			settings.Config.MaxConnectionAge
		};
		return [&nodes = settings.Nodes, importanceRetriever = settings.ImportanceRetriever, selectionConfig]() {
			return SelectNodesForRemoval(nodes, selectionConfig, importanceRetriever);
		};
	}

	thread::Task CreateAgePeersTask(const SelectorSettings& settings, net::ConnectionContainer& connectionContainer) {
		auto selector = CreateRemoveOnlyNodeSelector(settings);
		return CreateAgePeersTask(settings, connectionContainer, selector);
	}

	thread::Task CreateAgePeersTask(
			const SelectorSettings& settings,
			net::ConnectionContainer& connectionContainer,
			const RemoveOnlyNodeSelector& selector) {
		auto serviceId = settings.ServiceId;
		auto ager = CreateNodeAger(serviceId, settings.Config, settings.Nodes);
		return thread::CreateNamedTask("age peers task", [serviceId, ager, selector, &connectionContainer]() {
			// 1. age all connections
			ager(connectionContainer.identities());

			// 2. select remove candidates
			auto removeCandidates = selector();

			// 3. process remove candidates
			for (const auto& key : removeCandidates)
				connectionContainer.closeOne(key);

			return thread::make_ready_future(thread::TaskResult::Continue);
		});
	}

	// endregion
}}
