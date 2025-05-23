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

#include "NodeDiscoveryService.h"
#include "BatchPeersRequestor.h"
#include "NodePingRequestor.h"
#include "PeersProcessor.h"
#include "nodediscovery/src/handlers/NodeDiscoveryHandlers.h"
#include "catapult/extensions/NetworkUtils.h"
#include "catapult/extensions/NodeInteractionUtils.h"
#include "catapult/extensions/ServiceLocator.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/ionet/PacketPayloadFactory.h"

namespace catapult { namespace nodediscovery {

	namespace {
		constexpr auto Service_Name = "nd.ping_requestor";

		thread::Task CreatePingTask(extensions::ServiceState& state) {
			return thread::CreateNamedTask("node discovery ping task", [packetPayloadSink = state.hooks().packetPayloadSink(), &state]() {
				auto pLocalNetworkNode = utils::UniqueToShared(ionet::PackNode(config::ToLocalNode(state.config())));
				packetPayloadSink(ionet::PacketPayloadFactory::FromEntity(ionet::PacketType::Node_Discovery_Push_Ping, pLocalNetworkNode));
				return thread::make_ready_future(thread::TaskResult::Continue);
			});
		}

		thread::Task CreatePeersTask(extensions::ServiceState& state, const handlers::NodesConsumer& nodesConsumer) {
			BatchPeersRequestor requestor(state.packetIoPickers(), nodesConsumer);
			return thread::CreateNamedTask("node discovery peers task", [config = state.config(), &nodes = state.nodes(), requestor]() {
				return requestor.findPeersOfPeers(config.Node.SyncTimeout).then([&nodes](auto&& future) {
					auto results = future.get();
					for (const auto& result : results)
						extensions::IncrementNodeInteraction(nodes, result);

					return thread::TaskResult::Continue;
				});
			});
		}

		handlers::NodeConsumer CreatePushNodeConsumer(extensions::ServiceState& state) {
			return [&nodeSubscriber = state.nodeSubscriber()](const auto& node) {
				nodeSubscriber.notifyNode(node);
			};
		}

		class NodeDiscoveryServiceRegistrar : public extensions::ServiceRegistrar {
		public:
			extensions::ServiceRegistrarInfo info() const override {
				return { "NodeDiscovery", extensions::ServiceRegistrarPhase::Post_Packet_Io_Pickers };
			}

			void registerServiceCounters(extensions::ServiceLocator& locator) override {
				locator.registerServiceCounter<NodePingRequestor>(Service_Name, "ACTIVE PINGS", [](const auto& requestor) {
					return requestor.numActiveConnections();
				});
				locator.registerServiceCounter<NodePingRequestor>(Service_Name, "TOTAL PINGS", [](const auto& requestor) {
					return requestor.numTotalRequests();
				});
				locator.registerServiceCounter<NodePingRequestor>(Service_Name, "SUCCESS PINGS", [](const auto& requestor) {
					return requestor.numSuccessfulRequests();
				});
			}

			void registerServices(extensions::ServiceLocator& locator, extensions::ServiceState& state) override {
				// create callbacks
				auto pushNodeConsumer = CreatePushNodeConsumer(state);

				// register services
				auto connectionSettings = extensions::GetConnectionSettings(state.config());
				auto pServiceGroup = state.pool().pushServiceGroup("node_discovery");
				auto pNodePingRequestor = pServiceGroup->pushService(
						CreateNodePingRequestor,
						locator.keyPair(),
						connectionSettings,
						state);

				locator.registerService(Service_Name, pNodePingRequestor);

				// set handlers
				auto& nodeContainer = state.nodes();
				auto& pingRequestor = *pNodePingRequestor;
				handlers::RegisterNodeDiscoveryPushPingHandler(state, pushNodeConsumer);
				handlers::RegisterNodeDiscoveryPullPingHandler(state);

				auto pingRequestInitiator = [&pingRequestor](const auto& node, const auto& callback) {
					return pingRequestor.beginRequest(node, callback);
				};
				PeersProcessor peersProcessor(nodeContainer, pingRequestInitiator, state, pushNodeConsumer);
				auto pushPeersHandler = [peersProcessor](const auto& candidateNodes) {
					peersProcessor.process(candidateNodes);
				};
				handlers::RegisterNodeDiscoveryPushPeersHandler(state.packetHandlers(), pushPeersHandler);
				handlers::RegisterNodeDiscoveryPullPeersHandler(state.packetHandlers(), [&nodeContainer]() {
					return ionet::FindAllActiveNodes(nodeContainer.view());
				});

				// add task
				state.tasks().push_back(CreatePingTask(state));
				state.tasks().push_back(CreatePeersTask(state, pushPeersHandler));
			}
		};
	}

	DECLARE_SERVICE_REGISTRAR(NodeDiscovery)() {
		return std::make_unique<NodeDiscoveryServiceRegistrar>();
	}
}}
