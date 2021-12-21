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

#include "ApiNodeHealthUtils.h"
#include "tools/NetworkCensusTool.h"
#include "tools/ToolMain.h"
#include "tools/ToolThreadUtils.h"
#include "catapult/api/RemoteChainApi.h"
#include "catapult/extensions/RemoteDiagnosticApi.h"
#include "catapult/utils/DiagnosticCounterId.h"
#include "catapult/utils/Functional.h"

// Network Census

#pragma once
#include "tools/Tool.h"
#include "tools/ToolConfigurationUtils.h"
#include "tools/ToolNetworkUtils.h"
#include "catapult/ionet/Node.h"
#include "catapult/thread/FutureUtils.h"

// prometheus

#include "sstream"
#include "prometheus/client_metric.h"
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/family.h"
#include "prometheus/registry.h"

namespace catapult { namespace tools { namespace monitor {

	namespace {
		// region node info

		struct NodeInfo {
		public:
			explicit NodeInfo(const ionet::Node& node) : Node(node)
			{}

		public:
			ionet::Node Node;
			Height ChainHeight;
			model::ChainScore ChainScore;
			model::EntityRange<model::DiagnosticCounterValue> DiagnosticCounters;
		};

		using NodeInfoPointer = NetworkCensusTool<NodeInfo>::NodeInfoPointer;

		// endregion

		// region futures

		thread::future<bool> CreateChainInfoFuture(thread::IoThreadPool& pool, ionet::PacketIo& io, NodeInfo& nodeInfo) {
			thread::future<api::ChainInfo> chainInfoFuture;
			if (!HasFlag(ionet::NodeRoles::Peer, nodeInfo.Node.metadata().Roles)) {
				chainInfoFuture = CreateApiNodeChainInfoFuture(pool, nodeInfo.Node);
			} else {
				auto pApi = api::CreateRemoteChainApiWithoutRegistry(io);
				chainInfoFuture = pApi->chainInfo();
			}

			return chainInfoFuture.then([&nodeInfo](auto&& infoFuture) {
				return UnwrapFutureAndSuppressErrors("querying chain info", std::move(infoFuture), [&nodeInfo](const auto& info) {
					nodeInfo.ChainHeight = info.Height;
					nodeInfo.ChainScore = info.Score;
				});
			});
		}

		thread::future<bool> CreateDiagnosticCountersFuture(ionet::PacketIo& io, NodeInfo& nodeInfo) {
			auto pApi = extensions::CreateRemoteDiagnosticApi(io);
			return pApi->diagnosticCounters().then([&nodeInfo](auto&& countersFuture) {
				UnwrapFutureAndSuppressErrors("querying diagnostic counters", std::move(countersFuture), [&nodeInfo](auto&& counters) {
					nodeInfo.DiagnosticCounters = std::move(counters);
				});
			});
		}

		// endregion
		
		class MonitoringTool : public Tool {
		public:
			std::string name() const override {
				return "Monitoring Tool";
			}

			int run(const Options& options) override {
				PrometheusHealthCheck();
				return 0;
			}

			void prepareOptions(OptionsBuilder& optionsBuilder, OptionsPositional& positional) override {
			optionsBuilder("resources,r",
					OptionsValue<std::string>(m_resourcesPath)->default_value(".."),
					"the path to the resources directory");
			positional.add("resources", -1);
		}

		private:
			std::vector<thread::future<bool>> getNodeInfoFutures(
					thread::IoThreadPool& pool,
					ionet::PacketIo& io,
					NodeInfo& nodeInfo) {
				std::vector<thread::future<bool>> infoFutures;
				infoFutures.emplace_back(CreateChainInfoFuture(pool, io, nodeInfo));
				infoFutures.emplace_back(CreateDiagnosticCountersFuture(io, nodeInfo));
				return infoFutures;
			}

			size_t processNodeInfos(const std::vector<NodeInfoPointer>& nodeInfos) {
				PrometheusHealthCheck();

				return utils::Sum(nodeInfos, [](const auto& pNodeInfo) {
					return Height() == pNodeInfo->ChainHeight ? 1u : 0;
				});
			}

			/// A node info shared pointer.
			using NodeInfoPointer = std::shared_ptr<NodeInfo>;

			/// A node info shared pointer future.
			using NodeInfoFuture = thread::future<NodeInfoPointer>;

		private:
			NodeInfoFuture createNodeInfoFuture(MultiNodeConnector& connector, const ionet::Node& node) {
				auto pNodeInfo = std::make_shared<NodeInfo>(node);
				return thread::compose(connector.connect(node), [this, node, pNodeInfo, &connector](auto&& ioFuture) {
					try {
						auto pIo = ioFuture.get();
						auto infoFutures = this->getNodeInfoFutures(connector.pool(), *pIo, *pNodeInfo);

						// capture pIo so that it stays alive until all dependent futures are complete
						return thread::when_all(std::move(infoFutures)).then([pIo, pNodeInfo](auto&&) {
							return pNodeInfo;
						});
					} catch (...) {
						// suppress
						CATAPULT_LOG(error) << node << " appears to be offline";
						return thread::make_ready_future(NodeInfoPointer(pNodeInfo));
					}
				});
			}

			void PrometheusHealthCheck() {
				auto config = LoadConfiguration(m_resourcesPath);
				auto p2pNodes = LoadPeers(m_resourcesPath, config.Immutable.NetworkIdentifier);
				auto apiNodes = LoadOptionalApiPeers(m_resourcesPath, config.Immutable.NetworkIdentifier);

				using namespace prometheus;
				//prometheus : create a http server running on port 8080
				Exposer exposer{"0.0.0.0:8080"};

				//prometheus : create a metric registry
				auto registry = std::make_shared<Registry>();

				//prometheus : add a counter family to the registry
				auto& packet_counter = BuildCounter()
				.Name("observed_blockchain")
				.Help("Value of observed packets")
				.Register(*registry);
				exposer.RegisterCollectable(registry);

				for (;;) {
					MultiNodeConnector connector;
					std::vector<NodeInfoFuture> nodeInfoFutures;
					auto addNodeInfoFutures = [this, &connector, &nodeInfoFutures](const auto& nodes) {
						for (const auto& node : nodes) {
							CATAPULT_LOG(debug) << "preparing to get stats from node " << node;
							nodeInfoFutures.push_back(this->createNodeInfoFuture(connector, node));
						}
					};

					addNodeInfoFutures(p2pNodes);
					addNodeInfoFutures(apiNodes);

					std::vector<NodeInfoPointer> nodeInfos;

					auto finalFuture = thread::when_all(std::move(nodeInfoFutures)).then([this, &nodeInfos, &packet_counter](auto&& allFutures) {
						for (auto& nodeInfoFuture : allFutures.get()) {
							nodeInfos.push_back(nodeInfoFuture.get());
						}

						for (const auto& pNodeInfo : nodeInfos) {

							std::this_thread::sleep_for(std::chrono::seconds(1));
							//get string representation
							std::stringstream ssoNode, ssoHeight, ssoScore;
							ssoNode << pNodeInfo->Node;
							std::string node (ssoNode.str());

							ssoHeight << pNodeInfo->ChainHeight;
							std::string height (ssoHeight.str());

							ssoScore << pNodeInfo->ChainScore;
							std::string score (ssoScore.str());

							//register node info to prometheus
							auto& node_counter = packet_counter.Add({{"NodeInfo", node},
							{"Height", height}, 
							{"Score", score},  
							{"Type", (HasFlag(ionet::NodeRoles::Api, pNodeInfo->Node.metadata().Roles) ? "API" : "P2P")}});

							double double_height;
							ssoHeight >> double_height;
							if (node_counter.Value() != double_height) {
								node_counter.Increment(double_height);
							}
						}
					});
				
					std::this_thread::sleep_for(std::chrono::seconds(50));
				}
			}

		private:
			std::string m_resourcesPath;
			std::string m_censusName;
		};
	}
}}}

int main(int argc, const char** argv) {
	catapult::tools::monitor::MonitoringTool tool;
	return catapult::tools::ToolMain(argc, argv, tool);
}