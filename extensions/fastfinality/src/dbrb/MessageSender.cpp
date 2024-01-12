/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MessageSender.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/thread/Future.h"
#include "catapult/thread/FutureUtils.h"

namespace catapult { namespace dbrb {

	MessageSender::MessageSender(std::weak_ptr<net::PacketWriters> pWriters, NodeRetreiver& nodeRetreiver)
		: m_pWriters(std::move(pWriters))
		, m_nodeRetreiver(nodeRetreiver)
	{}

	void MessageSender::send(const std::shared_ptr<MessagePacket>& pPacket, const std::set<ProcessId>& recipients) {
		CATAPULT_LOG(trace) << "[DBRB] disseminating " << *pPacket << " to " << recipients.size() << " node(s)";
		getPacketIos(recipients);
		std::vector<thread::future<bool>> completionStatusFutures;
		for (const auto& recipient : recipients) {
			auto result = m_nodeRetreiver.getNode(recipient);
			if (result) {
				const auto& node = result.value();
				auto iter = m_packetIoPairs.find(node.identityKey());
				if (iter != m_packetIoPairs.end()) {
					CATAPULT_LOG(trace) << "[DBRB] sending " << *pPacket << " to " << node;
					auto pPromise = std::make_shared<thread::promise<bool>>();
					completionStatusFutures.push_back(pPromise->get_future());
					iter->second->io()->write(ionet::PacketPayload(pPacket), [pThisWeak = weak_from_this(), node, pPacket, pPromise](ionet::SocketOperationCode code) {
						if (code != ionet::SocketOperationCode::Success) {
							CATAPULT_LOG(error) << "[DBRB] sending " << *pPacket << " to " << node << " completed with " << code;
							auto pThis = pThisWeak.lock();
							if (pThis) {
								pThis->m_packetIoPairs.erase(node.identityKey());
								pThis->m_nodeRetreiver.removeNode(node.identityKey());
							}
						}
						pPromise->set_value(true);
					});
				} else {
					CATAPULT_LOG(debug) << "[DBRB] not connected to host " << node << ", skip sending " << *pPacket;
				}
			}
		}

		if (!completionStatusFutures.empty()) {
			thread::when_all(std::move(completionStatusFutures)).then([](auto&& completedFutures) {
				return thread::get_all_ignore_exceptional(completedFutures.get());
			}).get();
		}
	}

	NodePacketIoPairMap& MessageSender::getPacketIos(const std::set<ProcessId>& recipients) {
		auto pWriters = m_pWriters.lock();
		if (!pWriters)
			return m_packetIoPairs;

		for (const auto& recipient : recipients) {
			if (m_packetIoPairs.find(recipient) == m_packetIoPairs.end()) {
				auto nodePacketIoPair = pWriters->pickOne(recipient);
				if (nodePacketIoPair.io())
					m_packetIoPairs.emplace(recipient, std::make_shared<ionet::NodePacketIoPair>(nodePacketIoPair));
			}
		}

		return m_packetIoPairs;
	}
}}