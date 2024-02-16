/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MessageSender.h"
#include "DbrbChainPackets.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/ionet/NetworkNode.h"
#include "catapult/ionet/NodeContainer.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/thread/Future.h"
#include "catapult/thread/FutureUtils.h"

namespace catapult { namespace dbrb {

	namespace {
		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			if (connectCode == net::PeerConnectCode::Accepted)
				return utils::LogLevel::Debug;
			else
				return utils::LogLevel::Warning;
		}
	}

	MessageSender::MessageSender(ionet::Node thisNode, std::weak_ptr<net::PacketWriters> pWriters, const ionet::NodeContainer& nodeContainer, bool broadcastThisNode)
		: m_pWriters(std::move(pWriters))
		, m_running(true)
		, m_clearQueue(false)
		, m_workerThread(&MessageSender::workerThreadFunc, this)
		, m_thisNode(std::move(thisNode))
		, m_nodeContainer(nodeContainer)
		, m_broadcastThisNode(broadcastThisNode)
	{}

	MessageSender::~MessageSender() {
		{
			std::lock_guard<std::mutex> guard(m_messageMutex);
			m_running = false;
		}
		m_condVar.notify_one();
		m_workerThread.join();
	}

	ionet::NodePacketIoPair MessageSender::getNodePacketIoPair(const ProcessId& id) {
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

	void MessageSender::pushNodePacketIoPair(const ProcessId& id, const ionet::NodePacketIoPair& nodePacketIoPair) {
		std::lock_guard<std::mutex> guard(m_messageMutex);
		m_packetIoPairs.emplace(id, nodePacketIoPair);
	}

	void MessageSender::enqueue(const Payload& payload, const std::set<ProcessId>& recipients) {
		{
			std::lock_guard<std::mutex> guard(m_messageMutex);
			m_clearQueue = false;
			m_buffer.emplace_back(payload, recipients);
		}
		m_condVar.notify_one();
	}

	void MessageSender::clearQueue() {
		std::lock_guard<std::mutex> guard(m_messageMutex);
		m_clearQueue = true;
		if (!m_buffer.empty()) {
			CATAPULT_LOG(debug) << "[MESSAGE SENDER] clearing the queue (" << m_buffer.size() << ")";
			m_buffer.clear();
		}
	}

	void MessageSender::workerThreadFunc() {
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

	void MessageSender::requestNodes(const std::set<ProcessId>& requestedIds) {
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
			addNodes(nodes);
			broadcastNodes(nodes);
		}
	}

	void MessageSender::addNodes(const std::vector<ionet::Node>& nodes) {
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
						if (pThis->m_broadcastThisNode)
							pThis->broadcastNodes({ pThis->m_thisNode });
					}

					{
						std::lock_guard<std::mutex> guard(pThis->m_nodeMutex);
						pThis->m_connectionInProgress.erase(node.identityKey());
					}
				}
			});
		}
	}

	void MessageSender::broadcastNodes(const std::vector<ionet::Node>& nodes) {
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


	void MessageSender::broadcastNodes(const Payload& payload) {
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

	void MessageSender::removeNode(const ProcessId& id) {
		std::lock_guard<std::mutex> guard(m_nodeMutex);

		auto nodeIter = m_nodes.find(id);
		if (nodeIter != m_nodes.end())
			m_nodes.erase(nodeIter);

		auto packetIter = m_broadcastedPackets.find(id);
		if (packetIter != m_broadcastedPackets.end())
			m_broadcastedPackets.erase(packetIter);
	}

	void MessageSender::clearBroadcastData() {
		std::lock_guard<std::mutex> guard(m_nodeMutex);
		m_broadcastedPackets.clear();
	}
}}