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

// prometheus

#include "sstream"
#include "prometheus/client_metric.h"
#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/family.h"
#include "prometheus/registry.h"

namespace catapult { namespace tools { namespace health {

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

		//region prometheus

		void PrometheusHealthCheck(const std::vector<NodeInfoPointer>& nodeInfos){
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
			}
		}
	
		//endregion

		class HealthTool : public NetworkCensusTool<NodeInfo> {
		public:
			HealthTool() : NetworkCensusTool("Health")
			{}

		private:
			std::vector<thread::future<bool>> getNodeInfoFutures(
					thread::IoThreadPool& pool,
					ionet::PacketIo& io,
					NodeInfo& nodeInfo) override {
				std::vector<thread::future<bool>> infoFutures;
				infoFutures.emplace_back(CreateChainInfoFuture(pool, io, nodeInfo));
				infoFutures.emplace_back(CreateDiagnosticCountersFuture(io, nodeInfo));
				return infoFutures;
			}

			size_t processNodeInfos(const std::vector<NodeInfoPointer>& nodeInfos) override {
				PrometheusHealthCheck(nodeInfos);
				
				return utils::Sum(nodeInfos, [](const auto& pNodeInfo) {
					return Height() == pNodeInfo->ChainHeight ? 1u : 0;
				});
			}

		private:
			std::string m_resourcesPath;
		};
	}
}}}

int main(int argc, const char** argv) {
	catapult::tools::health::HealthTool tool;
	return catapult::tools::ToolMain(argc, argv, tool);
}