/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "fastfinality/src/dbrb/MessageSender.h"
#include "fastfinality/src/FastFinalityChainPackets.h"
#include "catapult/api/ApiTypes.h"
#include "catapult/dbrb/View.h"
#include "catapult/harvesting_core/HarvesterBlockGenerator.h"
#include "catapult/ionet/NodePacketIoPair.h"
#include "catapult/model/Elements.h"
#include "catapult/thread/FutureUtils.h"
#include <boost/asio/post.hpp>

namespace catapult {
	namespace harvesting {
		class UnlockedAccounts;
		struct HarvestingConfiguration;
	}
	namespace extensions {
		class ServiceState;
	}
	namespace handlers {
		struct PullBlocksHandlerConfiguration;
	}
}

namespace catapult { namespace fastfinality {

	std::shared_ptr<harvesting::UnlockedAccounts> CreateUnlockedAccounts(const harvesting::HarvestingConfiguration& config);
	harvesting::BlockGenerator CreateHarvesterBlockGenerator(extensions::ServiceState& state);
	handlers::PullBlocksHandlerConfiguration CreatePullBlocksHandlerConfiguration(const config::NodeConfiguration& nodeConfig);

	template<typename TFsm>
	RemoteNodeStateRetriever CreateRemoteNodeStateRetriever(
			const std::weak_ptr<TFsm>& pFsmWeak,
			const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder,
			const model::BlockElementSupplier& lastBlockElementSupplier) {
		return [pFsmWeak, pConfigHolder, lastBlockElementSupplier]() {
			auto pFsmShared = pFsmWeak.lock();
			if (!pFsmShared || pFsmShared->stopped())
				return std::vector<RemoteNodeState>();

			auto pPromise = std::make_shared<thread::promise<std::vector<RemoteNodeState>>>();

			boost::asio::post(pFsmShared->dbrbProcess().strand(), [pFsmWeak, pConfigHolder, lastBlockElementSupplier, pPromise]() {
				auto pFsmShared = pFsmWeak.lock();
				const auto& dbrbProcess = pFsmShared->dbrbProcess();
				if (!pFsmShared || pFsmShared->stopped()) {
					CATAPULT_LOG(warning) << "aborting node states retrieval";
					pPromise->set_value({});
					return;
				}

				auto chainHeight = lastBlockElementSupplier()->Block.Height;
				const auto& config = pConfigHolder->Config(chainHeight);
				auto view = config.Network.DbrbBootstrapProcesses;
				auto maxUnreachableNodeCount = dbrb::View::maxInvalidProcesses(view.size());
				view.erase(dbrbProcess.id());

				auto pMessageSender = dbrbProcess.messageSender();
				auto unreachableNodeCount = pMessageSender->getUnreachableNodeCount(view);
				if (unreachableNodeCount > maxUnreachableNodeCount) {
					CATAPULT_LOG(warning) << "unreachable node count " << unreachableNodeCount << " exceeds the limit " << maxUnreachableNodeCount;
					pPromise->set_value({});
					return;
				}

				const auto maxBlocksPerSyncAttempt = config.Node.MaxBlocksPerSyncAttempt;
				const auto targetHeight = chainHeight + Height(maxBlocksPerSyncAttempt);

				std::vector<RemoteNodeState> nodeStates;
				std::mutex mutex;
				auto pReadyPromise = std::make_shared<std::promise<bool>>();
				pFsmShared->packetHandlers().registerRemovableHandler(ionet::PacketType::Pull_Remote_Node_State_Response, [&nodeStates, &mutex, pReadyPromise, count = view.size()](
						const auto& packet, auto& context) {
					std::lock_guard<std::mutex> guard(mutex);
					const auto* pResponse = static_cast<const RemoteNodeStatePacket*>(&packet);
					const auto* pResponseData = reinterpret_cast<const Key*>(pResponse + 1);

					RemoteNodeState state;
					state.NodeKey = context.key();
					state.Height = pResponse->Height;
					state.BlockHash = pResponse->BlockHash;
					state.NodeWorkState = pResponse->NodeWorkState;
					for (auto i = 0; i < pResponse->HarvesterKeysCount; ++i)
						state.HarvesterKeys.push_back(pResponseData[i]);
					nodeStates.push_back(state);

					if (nodeStates.size() == count)
						pReadyPromise->set_value(true);
				});
				auto pPacket = ionet::CreateSharedPacket<RemoteNodeStatePacket>();
				pPacket->Height = targetHeight;
				pMessageSender->enqueue(pPacket, false, view);
				pReadyPromise->get_future().template wait_for(std::chrono::seconds(5));

				pFsmShared->packetHandlers().removeHandler(ionet::PacketType::Pull_Remote_Node_State_Response);

				auto minOpinionNumber = dbrb::View{ view }.quorumSize();
				CATAPULT_LOG(debug) << "retrieved " << nodeStates.size() << " node states, min opinion number " << minOpinionNumber;

				if (nodeStates.size() < minOpinionNumber)
					nodeStates.clear();

				pPromise->set_value(std::move(nodeStates));
			});

			auto value = pPromise->get_future().get();

			return value;
		};
	}
}}