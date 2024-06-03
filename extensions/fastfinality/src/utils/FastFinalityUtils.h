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

	class RemoteRequestDispatcher {
	public:
		/// Creates a remote request dispatcher around \a io.
		explicit RemoteRequestDispatcher(ionet::NodePacketIoPair nodePacketIoPair, dbrb::MessageSender& messageSender)
			: m_nodePacketIoPair(std::move(nodePacketIoPair))
			, m_messageSender(messageSender)
		{}

	public:
		const ionet::NodePacketIoPair& nodePacketIoPair() {
			return m_nodePacketIoPair;
		}

	public:
		/// Dispatches \a args to the underlying io.
		template<typename TFuncTraits, typename... TArgs>
		thread::future<typename TFuncTraits::ResultType> dispatch(const TFuncTraits& traits, TArgs&&... args) {
			auto pPromise = std::make_shared<thread::promise<typename TFuncTraits::ResultType>>();
			auto future = pPromise->get_future();
			auto packetPayload = TFuncTraits::CreateRequestPacketPayload(std::forward<TArgs>(args)...);
			send(traits, packetPayload, [pPromise](auto result, auto&& value) {
				if (RemoteChainResult::Success == result) {
					pPromise->set_value(std::forward<decltype(value)>(value));
					return;
				}

				std::ostringstream message;
				message << GetErrorMessage(result) << " for " << TFuncTraits::Friendly_Name << " request";
				CATAPULT_LOG(error) << message.str();
				pPromise->set_exception(std::make_exception_ptr(api::catapult_api_error(message.str().data())));
			});

			return future;
		}

	private:
		template<typename TFuncTraits, typename TCallback>
		void send(const TFuncTraits& traits, const ionet::PacketPayload& packetPayload, const TCallback& callback) {
			using ResultType = typename TFuncTraits::ResultType;
			m_nodePacketIoPair.io()->write(packetPayload, [traits, callback, &io = *m_nodePacketIoPair.io(), &node = m_nodePacketIoPair.node(), &messageSender = m_messageSender](auto code) {
				if (ionet::SocketOperationCode::Success != code) {
					CATAPULT_LOG(trace) << "[REQUEST DISPATCHER] removing node " << node << " " << node.identityKey();
					messageSender.removeNode(node.identityKey());
					return callback(RemoteChainResult::Write_Error, ResultType());
				}

				io.read([traits, callback, &node, &messageSender](auto readCode, const auto* pResponsePacket) {
					if (ionet::SocketOperationCode::Success != readCode) {
						CATAPULT_LOG(trace) << "[REQUEST DISPATCHER] removing node " << node << " " << node.identityKey();
						messageSender.removeNode(node.identityKey());
						return callback(RemoteChainResult::Read_Error, ResultType());
					}

					if (TFuncTraits::Packet_Type != pResponsePacket->Type) {
						CATAPULT_LOG(warning) << "received packet of type " << pResponsePacket->Type << " but expected type " << TFuncTraits::Packet_Type;
						return callback(RemoteChainResult::Malformed_Packet, ResultType());
					}

					ResultType result;
					if (!traits.tryParseResult(*pResponsePacket, result)) {
						CATAPULT_LOG(warning) << "unable to parse " << pResponsePacket->Type << " packet (size = " << pResponsePacket->Size << ")";
						return callback(RemoteChainResult::Malformed_Packet, ResultType());
					}

					return callback(RemoteChainResult::Success, std::move(result));
				});
			});
		}

	private:
		enum class RemoteChainResult {
			Success,
			Write_Error,
			Read_Error,
			Malformed_Packet
		};

		static constexpr const char* GetErrorMessage(RemoteChainResult result) {
			switch (result) {
			case RemoteChainResult::Write_Error:
				return "write to remote node failed";
			case RemoteChainResult::Read_Error:
				return "read from remote node failed";
			default:
				return "remote node returned malformed packet";
			}
		}

	private:
		ionet::NodePacketIoPair m_nodePacketIoPair;
		dbrb::MessageSender& m_messageSender;
	};

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
				if (!pFsmShared || pFsmShared->stopped() || pFsmShared->dbrbProcess().currentView().Data.empty()) {
					CATAPULT_LOG(warning) << "aborting node states retrieval, current view is empty";
					pPromise->set_value({});
					return;
				}

				const auto& dbrbProcess = pFsmShared->dbrbProcess();
				auto view = dbrbProcess.currentView();
				auto maxUnreachableNodeCount = dbrb::View::maxInvalidProcesses(view.Data.size());
				view.Data.erase(dbrbProcess.id());

				auto pMessageSender = dbrbProcess.messageSender();
				pMessageSender->clearQueue();
				pMessageSender->requestNodes(view.Data, pConfigHolder);

				auto unreachableNodeCount = pMessageSender->getUnreachableNodeCount(view.Data);
				if (unreachableNodeCount > maxUnreachableNodeCount) {
					CATAPULT_LOG(warning) << "unreachable node count " << unreachableNodeCount << " exceeds the limit " << maxUnreachableNodeCount;
					pPromise->set_value({});
					return;
				}

				auto chainHeight = lastBlockElementSupplier()->Block.Height;
				const auto& config = pConfigHolder->Config(chainHeight);
				const auto maxBlocksPerSyncAttempt = config.Node.MaxBlocksPerSyncAttempt;
				const auto targetHeight = chainHeight + Height(maxBlocksPerSyncAttempt);

				std::vector<thread::future<RemoteNodeState>> remoteNodeStateFutures;
				for (const auto& identityKey : view.Data) {
					auto nodePacketIoPair = pMessageSender->getNodePacketIoPair(identityKey);
					if (nodePacketIoPair) {
						auto pDispatcher = std::make_shared<RemoteRequestDispatcher>(std::move(nodePacketIoPair), *pMessageSender);
						remoteNodeStateFutures.push_back(pDispatcher->dispatch(RemoteNodeStateTraits{}, targetHeight).then([pMessageSender, pDispatcher, identityKey](auto&& stateFuture) {
							auto remoteNodeState = stateFuture.get();
							remoteNodeState.NodeKey = identityKey;
							pMessageSender->pushNodePacketIoPair(identityKey, pDispatcher->nodePacketIoPair());
							return remoteNodeState;
						}));
					} else {
						CATAPULT_LOG(debug) << "got no packet io to request node state from " << identityKey;
					}
				}

				std::vector<RemoteNodeState> nodeStates;
				if (!remoteNodeStateFutures.empty()) {
					nodeStates = thread::when_all(std::move(remoteNodeStateFutures)).then([](auto&& completedFutures) {
						return thread::get_all_ignore_exceptional(completedFutures.get());
					}).get();
				}

				auto minOpinionNumber = view.quorumSize();
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