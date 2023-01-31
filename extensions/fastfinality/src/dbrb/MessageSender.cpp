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
		constexpr auto Default_Timeout = utils::TimeSpan::FromMinutes(1);

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
			auto nodePacketIoPair = writers.pickOne(Default_Timeout, node.identityKey());
			if (nodePacketIoPair.io())
				return nodePacketIoPair;

			auto identities = writers.identities();
			auto iter = identities.find(node.identityKey());
			if (iter == identities.end()) {
				result = Connect(writers, node);
				if (result.Code == net::PeerConnectCode::Accepted)
					return writers.pickOne(Default_Timeout, node.identityKey());
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

	void MessageSender::processBuffer(BufferType& buffer) {
		BufferType unsentMessages;
		std::set<ProcessId> notFoundNodes;
		net::PeerConnectResult peerConnectResult;
		for (const auto& [pPacket, recipients] : buffer) {
			std::set<ProcessId> unprocessedNodes;
			for (const auto& recipient : recipients) {
				auto signedNode = m_nodeRetreiver.getNode(recipient);
				if (signedNode) {
					const auto& node = signedNode.value().Node;
					auto nodePacketIoPair = GetNodePacketIoPair(*m_pWriters, node, peerConnectResult);
					if (nodePacketIoPair.io()) {
						CATAPULT_LOG(debug) << "[DBRB] sending " << *pPacket << " to " << node;
						auto pPromise = std::make_shared<std::promise<ionet::SocketOperationCode>>();
						nodePacketIoPair.io()->write(ionet::PacketPayload(pPacket), [pPromise](ionet::SocketOperationCode code) {
							pPromise->set_value(code);
						});
						auto code = pPromise->get_future().get();
						if (code != ionet::SocketOperationCode::Success)
							CATAPULT_LOG(error) << "[DBRB] sending " << *pPacket << " to " << node << " completed with " << code;
					} else {
						if (peerConnectResult.Code == net::PeerConnectCode::Already_Connected)
							unprocessedNodes.emplace(recipient);
					}
				} else {
					notFoundNodes.emplace(recipient);
				}
			}

			if (!unprocessedNodes.empty())
				unsentMessages.emplace_back(pPacket, unprocessedNodes);
		}

		buffer.clear();

		{
			std::lock_guard<std::mutex> guard(m_mutex);
			unsentMessages.insert(unsentMessages.end(), std::make_move_iterator(m_buffer.begin()), std::make_move_iterator(m_buffer.end()));
			m_buffer.clear();
			std::swap(unsentMessages, m_buffer);
		}

		m_nodeRetreiver.enqueue(notFoundNodes);
	}
}}