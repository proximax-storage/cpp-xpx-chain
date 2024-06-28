/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MessageSender.h"
#include "DbrbChainPackets.h"
#include "TransactionSender.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/thread/Future.h"
#include "catapult/thread/FutureUtils.h"
#include "catapult/thread/IoThreadPool.h"
#include "catapult/utils/NetworkTime.h"
#include <condition_variable>
#include <thread>
#include <boost/asio/system_timer.hpp>

namespace catapult { namespace dbrb {

	namespace {
		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			if (connectCode == net::PeerConnectCode::Accepted)
				return utils::LogLevel::Debug;
			else
				return utils::LogLevel::Warning;
		}

		struct Message {
			dbrb::Payload Payload;
			bool DropOnFailure;
		};

		class MessageGroup {
		public:
			bool add(const ProcessId& recipient, const Message& message) {
				if (!m_recipients.emplace(recipient).second)
					return false;

				m_messages.emplace_back(recipient, message);

				return true;
			}

			const std::vector<std::pair<ProcessId, Message>> messages() const {
				return m_messages;
			}

		private:
			std::set<ProcessId> m_recipients;
			std::vector<std::pair<ProcessId, Message>> m_messages;
		};

		class MessageBuffer {
		public:
			MessageBuffer() : m_size(0) {}

		public:
			void enqueue(const Payload& payload, bool dropOnFailure, const std::set<ProcessId>& recipients) {
				Message message{ payload, dropOnFailure };
				for (const auto& recipient : recipients) {
					bool groupNotFound = true;
					for (auto& messageGroup : m_buffer) {
						if (messageGroup.add(recipient, message)) {
							groupNotFound = false;
							break;
						}
					}

					if (groupNotFound) {
						MessageGroup group;
						group.add(recipient, message);
						m_buffer.emplace_back(group);
					}

					m_size++;
				}
			}

			const std::vector<MessageGroup>& groups() const {
				return m_buffer;
			}

			size_t size() const {
				return m_size;
			}

			bool empty() const {
				return !m_size;
			}

			void clear() {
				m_buffer.clear();
				m_size = 0;
			}

		private:
			std::vector<MessageGroup> m_buffer;
			size_t m_size;
		};

		class DefaultMessageSender : public MessageSender, public std::enable_shared_from_this<DefaultMessageSender> {
		public:
			explicit DefaultMessageSender(
				ionet::Node thisNode,
				std::weak_ptr<net::PacketWriters> pWriters,
				const ionet::NodeContainer& nodeContainer,
				bool broadcastThisNode,
				std::shared_ptr<TransactionSender> pTransactionSender,
				std::shared_ptr<thread::IoThreadPool> pPool,
				const utils::TimeSpan& resendMessagesInterval);
			~DefaultMessageSender() override;

			// Message sending
			void enqueue(const Payload& payload, bool dropOnFailure, const std::set<ProcessId>& recipients) override;
			void clearQueue() override;
			ionet::NodePacketIoPair getNodePacketIoPair(const ProcessId& id) override;
			void pushNodePacketIoPair(const ProcessId& id, const ionet::NodePacketIoPair& nodePacketIoPair) override;

		private:
			void resendMessages();
			void workerThreadFunc();

		public:
			// Node discovery
			void findNodes(const std::set<ProcessId>& requestedIds) override;
			void addNodes(const std::vector<ionet::Node>& nodes) override;
			void sendNodes(const std::vector<ionet::Node>& nodes, const ProcessId& recipient) override;
			void removeNode(const ProcessId& id) override;
			bool isNodeAdded(const ProcessId& id) override;
			void addRemoveNodeResponse(const ProcessId& idToRemove, const ProcessId& respondentId, const Timestamp& timestamp, const Signature& signature) override;
			ViewData getUnreachableNodes(ViewData& view) const override;
			size_t getUnreachableNodeCount(const dbrb::ViewData& view) const override;
			std::vector<ionet::Node> getKnownNodes(ViewData& view) const override;

		private:
			void requestNodes(const std::set<ProcessId>& requestedIds);
			void broadcast(const Payload& payload, bool dropOnFailure);

		public:
			void clearNodeRemovalData() override;

		private:
			std::weak_ptr<net::PacketWriters> m_pWriters;
			std::map<ProcessId, ionet::NodePacketIoPair> m_packetIoPairs;

			// Message sending
			MessageBuffer m_buffer;
			MessageBuffer m_failedMessageBuffer;
			std::mutex m_messageMutex;
			std::condition_variable m_condVar;
			volatile std::atomic_bool m_running;
			volatile std::atomic_bool m_clearQueue;
			std::thread m_workerThread;
			std::shared_ptr<thread::IoThreadPool> m_pPool;
			boost::asio::system_timer m_timer;
			std::chrono::milliseconds m_resendMessagesInterval;


			// Node discovery
			ionet::Node m_thisNode;
			bool m_broadcastThisNode;
			const ionet::NodeContainer& m_nodeContainer;
			std::map<ProcessId, ionet::Node> m_nodes;
			std::unordered_set<ProcessId, utils::ArrayHasher<ProcessId>> m_connectionInProgress;
			mutable std::mutex m_nodeMutex;

			// Node removing
			struct NodeRemovalData {
				catapult::Timestamp Timestamp;
				ViewData View;
				std::map<ProcessId, Signature> Votes;
			};
			std::map<ProcessId, NodeRemovalData> m_nodesToRemove;
			std::shared_ptr<TransactionSender> m_pTransactionSender;
			mutable std::mutex m_removeNodeMutex;
		};
	}

	std::shared_ptr<MessageSender> CreateMessageSender(
			ionet::Node thisNode,
			std::weak_ptr<net::PacketWriters> pWriters,
			const ionet::NodeContainer& nodeContainer,
			bool broadcastThisNode,
			std::shared_ptr<TransactionSender> pTransactionSender,
			const std::shared_ptr<thread::IoThreadPool>& pPool,
			const utils::TimeSpan& resendMessagesInterval) {
		return std::make_shared<DefaultMessageSender>(std::move(thisNode), std::move(pWriters), nodeContainer, broadcastThisNode, std::move(pTransactionSender), pPool, resendMessagesInterval);
	}

	DefaultMessageSender::DefaultMessageSender(
			ionet::Node thisNode,
			std::weak_ptr<net::PacketWriters> pWriters,
			const ionet::NodeContainer& nodeContainer,
			bool broadcastThisNode,
			std::shared_ptr<TransactionSender> pTransactionSender,
			std::shared_ptr<thread::IoThreadPool> pPool,
			const utils::TimeSpan& resendMessagesInterval)
		: m_pWriters(std::move(pWriters))
		, m_running(true)
		, m_clearQueue(false)
		, m_workerThread(&DefaultMessageSender::workerThreadFunc, this)
		, m_thisNode(std::move(thisNode))
		, m_nodeContainer(nodeContainer)
		, m_broadcastThisNode(broadcastThisNode)
		, m_pTransactionSender(std::move(pTransactionSender))
		, m_pPool(std::move(pPool))
		, m_timer(m_pPool->ioContext())
		, m_resendMessagesInterval(resendMessagesInterval.millis())
	{}

	DefaultMessageSender::~DefaultMessageSender() {
		{
			std::lock_guard<std::mutex> guard(m_messageMutex);
			m_running = false;
		}
		m_timer.cancel();
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

	void DefaultMessageSender::enqueue(const Payload& payload, bool dropOnFailure, const std::set<ProcessId>& recipients) {
		{
			std::lock_guard<std::mutex> guard(m_messageMutex);
			m_clearQueue = false;
			m_buffer.enqueue(payload, dropOnFailure, recipients);
		}
		m_condVar.notify_one();
	}

	void DefaultMessageSender::clearQueue() {
		std::lock_guard<std::mutex> guard(m_messageMutex);
		m_clearQueue = true;

		if (!m_buffer.empty()) {
			CATAPULT_LOG(trace) << "[MESSAGE SENDER] clearing the queue (" << m_buffer.size() << ")";
			m_buffer.clear();
		}

		if (!m_failedMessageBuffer.empty()) {
			CATAPULT_LOG(trace) << "[MESSAGE SENDER] clearing failed messages (" << m_failedMessageBuffer.size() << ")";
			m_failedMessageBuffer.clear();
		}
	}

	void DefaultMessageSender::resendMessages() {
		{
			std::lock_guard<std::mutex> guard(m_messageMutex);
			if (m_failedMessageBuffer.empty())
				return;

			CATAPULT_LOG(trace) << "[MESSAGE SENDER] resending " << m_failedMessageBuffer.size() << " message(s)";

			for (const auto& messageGroup : m_failedMessageBuffer.groups()) {
				for (const auto& [recipient, message] : messageGroup.messages())
					m_buffer.enqueue(message.Payload, message.DropOnFailure, { recipient });
			}

			m_failedMessageBuffer.clear();
		}
		m_condVar.notify_one();
	}

	void DefaultMessageSender::workerThreadFunc() {
		while (m_running) {
			MessageBuffer buffer;
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

			for (const auto& messageGroup : buffer.groups()) {
				std::vector<thread::future<bool>> completionStatusFutures;
				for (const auto& pair : messageGroup.messages()) {
					const auto& recipient = pair.first;
					const auto& message = pair.second;
					auto nodePacketIoPair = getNodePacketIoPair(recipient);
					if (nodePacketIoPair) {
						CATAPULT_LOG(trace) << "[MESSAGE SENDER] sending " << *message.Payload << " to " << nodePacketIoPair.node() << " " << nodePacketIoPair.node().identityKey();
						auto pPromise = std::make_shared<thread::promise<bool>>();
						completionStatusFutures.push_back(pPromise->get_future());
						auto pPacketIoPair = std::make_shared<ionet::NodePacketIoPair>(nodePacketIoPair);
						nodePacketIoPair.io()->write(ionet::PacketPayload(message.Payload), [pThisWeak = weak_from_this(), message, pPromise, recipient, pPacketIoPair](ionet::SocketOperationCode code) {
							auto pThis = pThisWeak.lock();
							if (code != ionet::SocketOperationCode::Success) {
								CATAPULT_LOG(warning) << "[MESSAGE SENDER] sending " << *message.Payload << " to " << pPacketIoPair->node() << " " << pPacketIoPair->node().identityKey() << " completed with " << code;
								if (pThis) {
									CATAPULT_LOG(debug) << "[MESSAGE SENDER] removing node " << recipient;
									pThis->removeNode(recipient);

									if (!message.DropOnFailure) {
										std::lock_guard<std::mutex> lock(pThis->m_messageMutex);
										pThis->m_failedMessageBuffer.enqueue(message.Payload, message.DropOnFailure, { recipient });
									}
								}
							} else {
								if (pThis)
									pThis->pushNodePacketIoPair(recipient, *pPacketIoPair);
							}
							pPromise->set_value(true);
						});
					} else {
						CATAPULT_LOG(trace) << "[MESSAGE SENDER] NOT FOUND packet io to send " << *message.Payload << " to " << recipient;
						if (!message.DropOnFailure) {
							std::lock_guard<std::mutex> lock(m_messageMutex);
							m_failedMessageBuffer.enqueue(message.Payload, message.DropOnFailure, { recipient });
						}
					}
				}

				if (!completionStatusFutures.empty()) {
					thread::when_all(std::move(completionStatusFutures)).then([](auto&& completedFutures) {
						return thread::get_all_ignore_exceptional(completedFutures.get());
					}).get();
				}
			}

			{
				std::lock_guard<std::mutex> guard(m_messageMutex);
				if (m_clearQueue) {
					if (!m_buffer.empty()) {
						CATAPULT_LOG(trace) << "[MESSAGE SENDER] clearing the queue (" << m_buffer.size() << ")";
						m_buffer.clear();
					}

					if (!m_failedMessageBuffer.empty()) {
						CATAPULT_LOG(trace) << "[MESSAGE SENDER] clearing failed messages (" << m_failedMessageBuffer.size() << ")";
						m_failedMessageBuffer.clear();
					}

					m_clearQueue = false;
				} else {
					if (!m_failedMessageBuffer.empty()) {
						m_timer.expires_after(m_resendMessagesInterval);
						m_timer.async_wait([pThisWeak = weak_from_this()](const boost::system::error_code& ec) {
							auto pThis = pThisWeak.lock();
							if (!pThis)
								return;

							if (ec) {
								if (ec == boost::asio::error::operation_aborted)
									return;

								CATAPULT_THROW_EXCEPTION(boost::system::system_error(ec));
							}

							pThis->resendMessages();
						});
					}
				}
			}
		}
	}

	void DefaultMessageSender::findNodes(const std::set<ProcessId>& requestedIds) {
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

		CATAPULT_LOG(debug) << "[MESSAGE SENDER] looking for " << ids.size() << " nodes";

		if (ids.empty())
			return;

		nodes = m_nodeContainer.view().getNodes(ids);
		nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](const auto& node) { return node.endpoint().Host.empty(); }), nodes.end());
		CATAPULT_LOG(debug) << "[MESSAGE SENDER] got " << nodes.size() << " discovered nodes";
		if (!nodes.empty())
			addNodes(nodes);

		for (const auto& node : nodes)
			ids.erase(node.identityKey());

		if (ids.empty())
			return;

		CATAPULT_LOG(debug) << "[MESSAGE SENDER] requesting " << ids.size() << " nodes";

		requestNodes(ids);
	}

	void DefaultMessageSender::requestNodes(const std::set<ProcessId>& requestedIds) {
		if (requestedIds.empty())
			return;

		auto pPacket = ionet::CreateSharedPacket<DbrbPullNodesPacket>(requestedIds.size() * ProcessId_Size);
		pPacket->NodeCount = utils::checked_cast<size_t, uint16_t>(requestedIds.size());
		auto* pBuffer = reinterpret_cast<uint8_t*>(pPacket.get() + 1);
		for (const auto& id : requestedIds) {
			memcpy(pBuffer, id.data(), ProcessId_Size);
			pBuffer += ProcessId_Size;
		}

		broadcast(pPacket, true);
	}

	void DefaultMessageSender::addNodes(const std::vector<ionet::Node>& nodes) {
		auto pWriters = m_pWriters.lock();
		if (!pWriters)
			return;

		for (auto& node : nodes) {
			const auto& id = node.identityKey();
			if (id == m_thisNode.identityKey())
				continue;

			const auto& endpoint = node.endpoint();
			if (!endpoint.DbrbPort) {
				CATAPULT_LOG(trace) << "[MESSAGE SENDER] no DBRB port " << node << " " << id;
				continue;
			}
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
			pWriters->connect(dbrbNode, [pThisWeak = weak_from_this(), dbrbNode, node](const auto& result) {
				CATAPULT_LOG_LEVEL(MapToLogLevel(result.Code)) << "[MESSAGE SENDER] connection attempt to " << dbrbNode << " " << dbrbNode.identityKey() << " completed with " << result.Code;
				auto pThis = pThisWeak.lock();
				if (pThis) {
					if (result.Code == net::PeerConnectCode::Accepted || result.Code == net::PeerConnectCode::Already_Connected) {
						{
							std::lock_guard<std::mutex> guard(pThis->m_nodeMutex);
							pThis->m_nodes[node.identityKey()] = node;
						}
						CATAPULT_LOG(debug) << "[MESSAGE SENDER] Added node " << dbrbNode << " " << dbrbNode.identityKey();
						if (pThis->m_broadcastThisNode) {
							CATAPULT_LOG(debug) << "[MESSAGE SENDER] sharing this node " << pThis->m_thisNode << " [dbrb port " << pThis->m_thisNode.endpoint().DbrbPort << "] " << pThis->m_thisNode.identityKey();
							pThis->sendNodes({ pThis->m_thisNode }, node.identityKey());
						} else {
							pThis->m_condVar.notify_one();
						}
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

	void DefaultMessageSender::sendNodes(const std::vector<ionet::Node>& nodes, const ProcessId& recipient) {
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

		enqueue(pPacket, true, { recipient });
	}

	void DefaultMessageSender::broadcast(const Payload& payload, bool dropOnFailure) {
		std::set<ProcessId> recipients;
		{
			std::lock_guard<std::mutex> guard(m_nodeMutex);
			for (const auto& [id, _] : m_nodes)
				recipients.emplace(id);
		}

		if (!recipients.empty())
			enqueue(payload, dropOnFailure, recipients);
	}

	void DefaultMessageSender::removeNode(const ProcessId& id) {
		std::lock_guard<std::mutex> guard(m_nodeMutex);

		auto nodeIter = m_nodes.find(id);
		if (nodeIter != m_nodes.end())
			m_nodes.erase(nodeIter);
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

	ViewData DefaultMessageSender::getUnreachableNodes(ViewData& view) const {
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

	size_t DefaultMessageSender::getUnreachableNodeCount(const dbrb::ViewData& view) const {
		std::lock_guard<std::mutex> guard(m_nodeMutex);
		size_t count = 0;
		for (auto iter = view.cbegin(); iter != view.cend(); ++iter) {
			if (m_nodes.find(*iter) == m_nodes.cend())
				count++;
		}

		return count;
	}

	std::vector<ionet::Node> DefaultMessageSender::getKnownNodes(ViewData& view) const {
		std::lock_guard<std::mutex> guard(m_nodeMutex);
		std::vector<ionet::Node> nodes;
		for (const auto& id : view) {
			auto iter = m_nodes.find(id);
			if (iter != m_nodes.cend()) {
				nodes.push_back(iter->second);
			} if (id == m_thisNode.identityKey()) {
				nodes.push_back(m_thisNode);
			}
		}

		return nodes;
	}
}}