/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MessageSender.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/net/PacketWriters.h"

namespace catapult { namespace dbrb {

	namespace {
		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			if (connectCode == net::PeerConnectCode::Accepted)
				return utils::LogLevel::Info;
			else
				return utils::LogLevel::Warning;
		}

		net::PeerConnectResult Connect(net::PacketWriters& writers, const ionet::Node& node) {
			auto pPromise = std::make_shared<std::promise<net::PeerConnectResult>>();
			writers.connect(node, [pPromise, node](const net::PeerConnectResult& result) {
				CATAPULT_LOG_LEVEL(MapToLogLevel(result.Code)) << "[DBRB] connection attempt to " << node << " completed with " << result.Code;
				pPromise->set_value(result);
			});

			return pPromise->get_future().get();
		}

		ionet::NodePacketIoPair GetNodePacketIoPair(net::PacketWriters& writers, const ionet::Node& node, net::PeerConnectResult& result) {
			auto nodePacketIoPair = writers.pickOne(node.identityKey());
			if (nodePacketIoPair.io())
				return nodePacketIoPair;

			auto identities = writers.identities();
			auto iter = identities.find(node.identityKey());
			if (iter == identities.end()) {
				result = Connect(writers, node);
				if (result.Code == net::PeerConnectCode::Accepted)
					return writers.pickOne(node.identityKey());
			} else {
				result = net::PeerConnectResult(net::PeerConnectCode::Already_Connected);
			}

			return {};
		}
	}

	MessageSender::MessageSender(std::shared_ptr<net::PacketWriters> pWriters, NodeRetreiver& nodeRetreiver)
		: m_pWriters(std::move(pWriters))
		, m_nodeRetreiver(nodeRetreiver)
	{}

	MessageSender::~MessageSender() {
		stop();
	}

	void MessageSender::processBuffer(BufferType& buffer) {
		net::PeerConnectResult peerConnectResult;
		while (!buffer.empty()) {
			std::map<ProcessId, std::set<std::shared_ptr<MessagePacket>>> messages;
			auto packetCount = 0u;
			for (const auto& [pPacket, recipients] : buffer) {
				for (const auto& recipient : recipients) {
					messages[recipient].emplace(pPacket);
					packetCount++;
				}
			}
			buffer.clear();

			CATAPULT_LOG(debug) << "[DBRB] disseminating " << packetCount << " packet(s) to " << messages.size() << " node(s)";

			std::set<ProcessId> notFoundNodes;
			auto packetCountNotSentConnectionBusy = 0u;
			auto packetCountNotSentNoConnection = 0u;
			for (const auto& [recipient, packets] : messages) {
				auto signedNode = m_nodeRetreiver.getNode(recipient);
				if (signedNode) {
					const auto& node = signedNode.value().Node;
					std::shared_ptr<ionet::NodePacketIoPair> pPacketIoPair;
					auto iter = m_packetIoPairs.find(node.identityKey());
					if (iter != m_packetIoPairs.end()) {
						pPacketIoPair = iter->second;
					} else {
						auto nodePacketIoPair = GetNodePacketIoPair(*m_pWriters, node, peerConnectResult);
						if (nodePacketIoPair.io()) {
							pPacketIoPair = std::make_shared<ionet::NodePacketIoPair>(nodePacketIoPair);
							m_packetIoPairs.emplace(node.identityKey(), pPacketIoPair);
						}
					}

					if (pPacketIoPair) {
						for (const auto& pPacket : packets) {
							CATAPULT_LOG(debug) << "[DBRB] sending " << *pPacket << " to " << node;
							pPacketIoPair->io()->write(ionet::PacketPayload(pPacket), [pThisWeak = weak_from_this(), node, pPacket](ionet::SocketOperationCode code) {
								if (code != ionet::SocketOperationCode::Success) {
									CATAPULT_LOG(error) << "[DBRB] sending " << *pPacket << " to " << node << " completed with " << code;
									auto pThis = pThisWeak.lock();
									if (pThis)
										pThis->m_packetIoPairs.erase(node.identityKey());
								}
							});
						}
					} else if (peerConnectResult.Code == net::PeerConnectCode::Already_Connected) {
						for (const auto& pPacket : packets) {
							buffer.emplace_back(pPacket, std::set { recipient });
							packetCountNotSentConnectionBusy++;
						}
					}
				} else {
					notFoundNodes.emplace(recipient);
					packetCountNotSentNoConnection += packets.size();
				}
			}

			if (packetCountNotSentConnectionBusy > 0)
				CATAPULT_LOG(debug) << "[DBRB] not disseminated " << packetCountNotSentConnectionBusy << " packet(s) because connection is busy";

			if (packetCountNotSentNoConnection > 0)
				CATAPULT_LOG(debug) << "[DBRB] rejected " << packetCountNotSentNoConnection << " packet(s) because there is no connection";

			{
				std::lock_guard<std::mutex> guard(m_mutex);
				buffer.insert(buffer.end(), std::make_move_iterator(m_buffer.begin()), std::make_move_iterator(m_buffer.end()));
				m_buffer.clear();
			}

			m_nodeRetreiver.enqueue(notFoundNodes);
		}
	}
}}