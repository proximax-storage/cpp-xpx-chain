/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MessageSender.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/net/PacketWriters.h"
#include <future>

namespace catapult { namespace dbrb {

	namespace {
		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			if (connectCode == net::PeerConnectCode::Accepted)
				return utils::LogLevel::Info;
			else
				return utils::LogLevel::Warning;
		}

		net::PeerConnectResult Connect(net::PacketWriters& writers, const ionet::Node& node) {
			CATAPULT_LOG(debug) << "[DBRB] Connecting to " << node;
			auto pPromise = std::make_shared<std::promise<net::PeerConnectResult>>();
			writers.connect(node, [pPromise, node](const net::PeerConnectResult& result) {
				CATAPULT_LOG_LEVEL(MapToLogLevel(result.Code)) << "[DBRB] connection attempt to " << node << " completed with " << result.Code;
				pPromise->set_value(result);
			});

			return pPromise->get_future().get();
		}

		ionet::NodePacketIoPair GetNodePacketIoPair(net::PacketWriters& writers, const ionet::Node& node) {
			auto nodePacketIoPair = writers.pickOne(node.identityKey());
			if (nodePacketIoPair.io())
				return nodePacketIoPair;

			auto identities = writers.identities();
			auto iter = identities.find(node.identityKey());
			if (iter == identities.end()) {
				auto result = Connect(writers, node);
				if (result.Code == net::PeerConnectCode::Accepted)
					return writers.pickOne(node.identityKey());
			}

			return {};
		}
	}

	MessageSender::MessageSender(std::weak_ptr<net::PacketWriters> pWriters, NodeRetreiver& nodeRetreiver)
		: m_pWriters(std::move(pWriters))
		, m_nodeRetreiver(nodeRetreiver)
	{}

	void MessageSender::send(const std::shared_ptr<MessagePacket>& pPacket, const std::set<ProcessId>& recipients) {
		CATAPULT_LOG(trace) << "[DBRB] disseminating " << *pPacket << " to " << recipients.size() << " node(s)";
		std::set<ProcessId> notFoundNodes;
		for (const auto& recipient : recipients) {
			auto signedNode = m_nodeRetreiver.getNode(recipient);
			if (signedNode) {
				const auto& node = signedNode.value().Node;
				std::shared_ptr<ionet::NodePacketIoPair> pPacketIoPair;
				auto iter = m_packetIoPairs.find(node.identityKey());
				if (iter != m_packetIoPairs.end()) {
					pPacketIoPair = iter->second;
				} else if (!node.endpoint().Host.empty()) {
					auto pWriters = m_pWriters.lock();
					if (pWriters) {
						auto nodePacketIoPair = GetNodePacketIoPair(*pWriters, node);
						if (nodePacketIoPair.io()) {
							pPacketIoPair = std::make_shared<ionet::NodePacketIoPair>(nodePacketIoPair);
							m_packetIoPairs.emplace(node.identityKey(), pPacketIoPair);
						}
					}
				} else {
					CATAPULT_LOG(debug) << "[DBRB] host is unknown, skipped sending " << *pPacket << " to " << node;
				}

				if (pPacketIoPair) {
					CATAPULT_LOG(trace) << "[DBRB] sending " << *pPacket << " to " << node;
					pPacketIoPair->io()->write(ionet::PacketPayload(pPacket), [pThisWeak = weak_from_this(), node, pPacket](ionet::SocketOperationCode code) {
						if (code != ionet::SocketOperationCode::Success) {
							CATAPULT_LOG(error) << "[DBRB] sending " << *pPacket << " to " << node << " completed with " << code;
							auto pThis = pThisWeak.lock();
							if (pThis)
								pThis->m_packetIoPairs.erase(node.identityKey());
						}
					});
				}
			} else {
				notFoundNodes.emplace(recipient);
			}
		}

		m_nodeRetreiver.requestNodes(notFoundNodes);
	}
}}