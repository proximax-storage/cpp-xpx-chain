/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MessageSender.h"
#include "DbrbChainPackets.h"
#include "TransactionSender.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/thread/Future.h"
#include "catapult/thread/FutureUtils.h"
#include "catapult/utils/NetworkTime.h"
#include <condition_variable>
#include <thread>

namespace catapult { namespace dbrb {

	namespace {
		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			if (connectCode == net::PeerConnectCode::Accepted)
				return utils::LogLevel::Debug;
			else
				return utils::LogLevel::Warning;
		}

		class DefaultMessageSender : public MessageSender, public std::enable_shared_from_this<DefaultMessageSender> {
		public:
			explicit DefaultMessageSender(
				ionet::Node thisNode,
				std::weak_ptr<net::PacketWriters> pWriters,
				const ionet::NodeContainer& nodeContainer,
				bool broadcastThisNode,
				std::shared_ptr<TransactionSender> pTransactionSender,
				const dbrb::DbrbViewFetcher& dbrbViewFetcher);
			~DefaultMessageSender() override;

			// Message sending
			void enqueue(const Payload& payload, const std::set<ProcessId>& recipients) override;
			void clearQueue() override;
			ionet::NodePacketIoPair getNodePacketIoPair(const ProcessId& id) override;
			void pushNodePacketIoPair(const ProcessId& id, const ionet::NodePacketIoPair& nodePacketIoPair) override;

		private:
			void workerThreadFunc();

		public:
			// Node discovery
			void requestNodes(const std::set<ProcessId>& requestedIds, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) override;
			void addNodes(const std::vector<ionet::Node>& nodes, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) override;
			void removeNode(const ProcessId& id) override;
			void broadcastNodes(const Payload& payload) override;
			void broadcastThisNode() override;
			void clearBroadcastData() override;
			bool isNodeAdded(const ProcessId& id) override;
			void addRemoveNodeResponse(const ProcessId& idToRemove, const ProcessId& respondentId, const Timestamp& timestamp, const Signature& signature) override;
			ViewData getUnreachableNodes(ViewData& view) override;

		private:
			void broadcastNodes(const std::vector<ionet::Node>& nodes);

		public:
			void clearNodeRemovalData() override;

		private:
			std::weak_ptr<net::PacketWriters> m_pWriters;
			NodePacketIoPairMap m_packetIoPairs;

			// Message sending
			BufferType m_buffer;
			std::mutex m_messageMutex;
			std::condition_variable m_condVar;
			volatile bool m_running;
			volatile bool m_clearQueue;
			std::thread m_workerThread;

			// Node discovery
			ionet::Node m_thisNode;
			bool m_broadcastThisNode;
			const ionet::NodeContainer& m_nodeContainer;
			std::map<ProcessId, ionet::Node> m_nodes;
			std::unordered_set<ProcessId, utils::ArrayHasher<ProcessId>> m_connectionInProgress;
			std::map<ProcessId, std::unordered_set<Hash256, utils::ArrayHasher<Hash256>>> m_broadcastedPackets;
			mutable std::mutex m_nodeMutex;

			// Node removing
			struct NodeRemovalData {
				catapult::Timestamp Timestamp;
				ViewData View;
				std::map<ProcessId, Signature> Votes;
			};
			std::map<ProcessId, NodeRemovalData> m_nodesToRemove;
			std::shared_ptr<TransactionSender> m_pTransactionSender;
			const dbrb::DbrbViewFetcher& m_dbrbViewFetcher;
			mutable std::mutex m_removeNodeMutex;
		};
	}

	std::shared_ptr<MessageSender> CreateMessageSender(
			ionet::Node thisNode,
			std::weak_ptr<net::PacketWriters> pWriters,
			const ionet::NodeContainer& nodeContainer,
			bool broadcastThisNode,
			std::shared_ptr<TransactionSender> pTransactionSender,
			const dbrb::DbrbViewFetcher& dbrbViewFetcher) {
		return std::make_shared<DefaultMessageSender>(std::move(thisNode), std::move(pWriters), nodeContainer, broadcastThisNode, std::move(pTransactionSender), dbrbViewFetcher);
	}

	DefaultMessageSender::DefaultMessageSender(
			ionet::Node thisNode,
			std::weak_ptr<net::PacketWriters> pWriters,
			const ionet::NodeContainer& nodeContainer,
			bool broadcastThisNode,
			std::shared_ptr<TransactionSender> pTransactionSender,
			const dbrb::DbrbViewFetcher& dbrbViewFetcher)
		: m_pWriters(std::move(pWriters))
		, m_running(true)
		, m_clearQueue(false)
		, m_workerThread(&DefaultMessageSender::workerThreadFunc, this)
		, m_thisNode(std::move(thisNode))
		, m_nodeContainer(nodeContainer)
		, m_broadcastThisNode(broadcastThisNode)
		, m_pTransactionSender(std::move(pTransactionSender))
		, m_dbrbViewFetcher(dbrbViewFetcher)
	{}

	DefaultMessageSender::~DefaultMessageSender() {
		{
			std::lock_guard<std::mutex> guard(m_messageMutex);
			m_running = false;
		}
		m_condVar.notify_one();
		m_workerThread.join();
	}

	ionet::NodePacketIoPair DefaultMessageSender::getNodePacketIoPair(const ProcessId& id) {
		std::lock_guard<std::mutex> guard(m_messageMutex);
		auto iter = m_packetIoPairs.find(id);
		if (iter != m_packetIoPairs.end()) {
			auto nodePacketIoPair = iter->second;
			m_packetIoPairs.erase(iter);

			return nodePacketIoPair;
		}

		auto pWriters = m_pWriters.lock();
		if (!pWriters)
			return {};

		return pWriters->pickOne(id);
	}

	void DefaultMessageSender::pushNodePacketIoPair(const ProcessId& id, const ionet::NodePacketIoPair& nodePacketIoPair) {
		std::lock_guard<std::mutex> guard(m_messageMutex);
		m_packetIoPairs.emplace(id, nodePacketIoPair);
	}

	void DefaultMessageSender::enqueue(const Payload& payload, const std::set<ProcessId>& recipients) {
		{
			std::lock_guard<std::mutex> guard(m_messageMutex);
			m_clearQueue = false;
			m_buffer.emplace_back(payload, recipients);
		}
		m_condVar.notify_one();
	}

	void DefaultMessageSender::clearQueue() {
		std::lock_guard<std::mutex> guard(m_messageMutex);
		m_clearQueue = true;
		if (!m_buffer.empty()) {
			CATAPULT_LOG(debug) << "[MESSAGE SENDER] clearing the queue (" << m_buffer.size() << ")";
			m_buffer.clear();
		}
	}

	void DefaultMessageSender::workerThreadFunc() {
		BufferType buffer;
		while (m_running) {
			{
				std::unique_lock<std::mutex> lock(m_messageMutex);
				m_condVar.wait(lock, [this] {
					return !m_buffer.empty() || !m_running;
				});

				if (m_clearQueue) {
					m_buffer.clear();
					m_clearQueue = false;
					continue;
				}

				std::swap(buffer, m_buffer);

				if (!m_running) {
					return;
				}
			}

			auto pWriters = m_pWriters.lock();
			if (!pWriters)
				return;

			net::PeerConnectResult peerConnectResult;
			for (const auto& pair : buffer) {
				const auto& pPacket = pair.first;
				const auto& recipients = pair.second;
				std::vector<thread::future<bool>> completionStatusFutures;
				for (const auto& recipient : recipients) {
					auto nodePacketIoPair = getNodePacketIoPair(recipient);
					if (nodePacketIoPair) {
						CATAPULT_LOG(trace) << "[MESSAGE SENDER] sending " << *pPacket << " to " << nodePacketIoPair.node() << " " << nodePacketIoPair.node().identityKey();
						auto pPromise = std::make_shared<thread::promise<bool>>();
						completionStatusFutures.push_back(pPromise->get_future());
						auto pPacketIoPair = std::make_shared<ionet::NodePacketIoPair>(nodePacketIoPair);
						nodePacketIoPair.io()->write(ionet::PacketPayload(pPacket), [pThisWeak = weak_from_this(), pPacket, pPromise, recipient, pPacketIoPair](ionet::SocketOperationCode code) {
							auto pThis = pThisWeak.lock();
							if (code != ionet::SocketOperationCode::Success) {
								CATAPULT_LOG(warning) << "[MESSAGE SENDER] sending " << *pPacket << " to " << pPacketIoPair->node() << " " << pPacketIoPair->node().identityKey() << " completed with " << code;
								if (pThis) {
									CATAPULT_LOG(debug) << "[MESSAGE SENDER] removing node " << recipient;
									pThis->removeNode(recipient);
								}
							} else {
								if (pThis)
									pThis->pushNodePacketIoPair(recipient, *pPacketIoPair);
							}
							pPromise->set_value(true);
						});
					} else {
						CATAPULT_LOG(trace) << "[MESSAGE SENDER] NOT FOUND packet io to send " << *pPacket << " to " << recipient;
					}
				}

				if (!completionStatusFutures.empty()) {
					thread::when_all(std::move(completionStatusFutures)).then([](auto&& completedFutures) {
						return thread::get_all_ignore_exceptional(completedFutures.get());
					}).get();
				}
			}

			buffer.clear();

			{
				std::lock_guard<std::mutex> guard(m_messageMutex);
				if (m_clearQueue) {
					if (!m_buffer.empty()) {
						CATAPULT_LOG(trace) << "[MESSAGE SENDER] clearing the queue (" << m_buffer.size() << ")";
						m_buffer.clear();
					}
					m_clearQueue = false;
				}
			}
		}
	}

	void DefaultMessageSender::requestNodes(const std::set<ProcessId>& requestedIds, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		std::vector<ionet::Node> nodes;
		std::set<ProcessId> ids;

		{
			std::lock_guard<std::mutex> guard(m_nodeMutex);
			for (const auto& id : requestedIds) {
				if (m_nodes.find(id) == m_nodes.end())
					ids.emplace(id);
			}
		}

		ids.erase(m_thisNode.identityKey());

		CATAPULT_LOG(debug) << "[MESSAGE SENDER] requesting " << ids.size() << " nodes";

		if (ids.empty())
			return;

		nodes = m_nodeContainer.view().getNodes(ids);
		nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](const auto& node) { return node.endpoint().Host.empty(); }), nodes.end());
		CATAPULT_LOG(debug) << "[MESSAGE SENDER] got " << nodes.size() << " discovered nodes";
		if (!nodes.empty()) {
			addNodes(nodes, pConfigHolder);
			broadcastNodes(nodes);
		}
	}

	void DefaultMessageSender::addNodes(const std::vector<ionet::Node>& nodes, const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		auto pWriters = m_pWriters.lock();
		if (!pWriters)
			return;

		for (auto& node : nodes) {
			const auto& id = node.identityKey();
			if (id == m_thisNode.identityKey())
				continue;

			const auto& endpoint = node.endpoint();
			auto dbrbNode = ionet::Node(node.identityKey(), ionet::NodeEndpoint{ endpoint.Host, endpoint.DbrbPort }, node.metadata());
			auto identities = pWriters->identities();
			if ((identities.find(id) != identities.cend())) {
				CATAPULT_LOG(trace) << "[MESSAGE SENDER] Already connected to " << dbrbNode << " " << id;
				std::lock_guard<std::mutex> guard(m_nodeMutex);
				m_nodes[id] = node;
				continue;
			}

			{
				std::lock_guard<std::mutex> guard(m_nodeMutex);
				if (m_connectionInProgress.find(id) == m_connectionInProgress.cend()) {
					m_connectionInProgress.emplace(id);
				} else {
					continue;
				}
			}

			CATAPULT_LOG(debug) << "[MESSAGE SENDER] Connecting to " << dbrbNode << " " << id;
			pWriters->connect(dbrbNode, [pThisWeak = weak_from_this(), dbrbNode, node, pConfigHolder](const auto& result) {
				CATAPULT_LOG_LEVEL(MapToLogLevel(result.Code)) << "[MESSAGE SENDER] connection attempt to " << dbrbNode << " " << dbrbNode.identityKey() << " completed with " << result.Code;
				auto pThis = pThisWeak.lock();
				if (pThis) {
					if (result.Code == net::PeerConnectCode::Accepted || result.Code == net::PeerConnectCode::Already_Connected) {
						{
							std::lock_guard<std::mutex> guard(pThis->m_nodeMutex);
							pThis->m_nodes[node.identityKey()] = node;
						}
						CATAPULT_LOG(debug) << "[MESSAGE SENDER] Added node " << dbrbNode << " " << dbrbNode.identityKey();
						pThis->broadcastThisNode();
					} else {
						// TODO: uncomment and retest
//						std::lock_guard<std::mutex> guard(pThis->m_removeNodeMutex);
//						auto timestamp = utils::NetworkTime();
//						auto view = View{ pThis->m_dbrbViewFetcher.getView(timestamp) };
//						auto nodeIter = pThis->m_nodesToRemove.find(node.identityKey());
//						if (nodeIter == pThis->m_nodesToRemove.cend() && view.isMember(node.identityKey())) {
//							view.Data.erase(node.identityKey());
//							auto bootstrapView = View{ pConfigHolder->Config().Network.DbrbBootstrapProcesses };
//							view.merge(bootstrapView);
//
//							if (!pThis->m_pTransactionSender->isRemoveDbrbProcessByNetworkTransactionSent(node.identityKey()) && view.isMember(pThis->m_thisNode.identityKey())) {
//								pThis->m_nodesToRemove.emplace(node.identityKey(), NodeRemovalData{ timestamp, view.Data, std::map<ProcessId, Signature>{} });
//								auto pRequest = ionet::CreateSharedPacket<DbrbRemoveNodeRequestPacket>();
//								pRequest->Timestamp = timestamp;
//								pRequest->ProcessId = node.identityKey();
//								view.Data.erase(pThis->m_thisNode.identityKey());
//								pThis->enqueue(pRequest, view.Data);
//							}
//						}
					}

					{
						std::lock_guard<std::mutex> guard(pThis->m_nodeMutex);
						pThis->m_connectionInProgress.erase(node.identityKey());
					}
				}
			});
		}
	}

	void DefaultMessageSender::broadcastThisNode() {
		if (m_broadcastThisNode)
			broadcastNodes({ m_thisNode });
	}

	void DefaultMessageSender::broadcastNodes(const std::vector<ionet::Node>& nodes) {
		if (nodes.empty())
			return;

		auto nodeCount = utils::checked_cast<size_t, uint16_t>(nodes.size());
		std::vector<model::UniqueEntityPtr<ionet::NetworkNode>> networkNodes;
		networkNodes.reserve(nodeCount);
		auto payloadSize = 0u;
		for (const auto& node : nodes) {
			networkNodes.emplace_back(ionet::PackNode(node));
			payloadSize += networkNodes.back()->Size;
		}

		auto pPacket = ionet::CreateSharedPacket<DbrbPushNodesPacket>(payloadSize);
		pPacket->NodeCount = nodeCount;
		auto* pBuffer = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		for (const auto& pNetworkNode : networkNodes) {
			memcpy(pBuffer, pNetworkNode.get(), pNetworkNode->Size);
			pBuffer += pNetworkNode->Size;
		}

		broadcastNodes(pPacket);
	}


	void DefaultMessageSender::broadcastNodes(const Payload& payload) {
		auto hash = CalculatePayloadHash(payload);

		std::set<ProcessId> ids;
		{
			std::lock_guard<std::mutex> guard(m_nodeMutex);
			for (const auto& pair : m_nodes) {
				auto& hashes = m_broadcastedPackets[pair.second.identityKey()];
				if (hashes.find(hash) == hashes.cend()) {
					hashes.emplace(hash);
					ids.emplace(pair.second.identityKey());
				}
			}
		}

		if (!ids.empty())
			enqueue(payload, ids);
	}

	void DefaultMessageSender::removeNode(const ProcessId& id) {
		std::lock_guard<std::mutex> guard(m_nodeMutex);

		auto nodeIter = m_nodes.find(id);
		if (nodeIter != m_nodes.end())
			m_nodes.erase(nodeIter);

		auto packetIter = m_broadcastedPackets.find(id);
		if (packetIter != m_broadcastedPackets.end())
			m_broadcastedPackets.erase(packetIter);
	}

	void DefaultMessageSender::clearBroadcastData() {
		std::lock_guard<std::mutex> guard(m_nodeMutex);
		m_broadcastedPackets.clear();
	}

	void DefaultMessageSender::clearNodeRemovalData() {
		std::lock_guard<std::mutex> guard(m_removeNodeMutex);
		m_nodesToRemove.clear();
	}

	bool DefaultMessageSender::isNodeAdded(const ProcessId& id) {
		std::lock_guard<std::mutex> guard(m_nodeMutex);
		return (m_nodes.find(id) != m_nodes.cend() || m_connectionInProgress.find(id) != m_connectionInProgress.cend());
	}

	void DefaultMessageSender::addRemoveNodeResponse(const ProcessId& idToRemove, const ProcessId& respondentId, const Timestamp& timestamp, const Signature& signature) {
		std::lock_guard<std::mutex> guard(m_removeNodeMutex);
		auto iter = m_nodesToRemove.find(idToRemove);
		if (iter == m_nodesToRemove.cend() || iter->second.Timestamp != timestamp || iter->second.View.find(respondentId) == iter->second.View.cend())
			return;

		auto& votes = iter->second.Votes;
		votes.emplace(respondentId, signature);

		// Add own vote.
		if (votes.size() + 1 != (View{ iter->second.View }.quorumSize()))
			return;

		m_pTransactionSender->sendRemoveDbrbProcessByNetworkTransaction(idToRemove, timestamp, votes);
	}

	ViewData DefaultMessageSender::getUnreachableNodes(ViewData& view) {
		std::lock_guard<std::mutex> guard(m_nodeMutex);
		ViewData unreachableNodes;
		for (auto iter = view.cbegin(); iter != view.cend();) {
			if (m_nodes.find(*iter) == m_nodes.cend()) {
				unreachableNodes.emplace(*iter);
				iter = view.erase(iter);
			} else {
				++iter;
			}
		}

		return unreachableNodes;
	}
}}