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

	namespace {
		std::string getNodeName(const NodeRetreiver& nodeRetreiver, const Key& identityKey) {
			std::string recipientStr;
			std::ostringstream out;
			auto node = nodeRetreiver.getNode(identityKey);
			if (node) {
				out << node.value();
			} else {
				out << identityKey;
			}

			return out.str();
		}
	}

	MessageSender::MessageSender(std::weak_ptr<net::PacketWriters> pWriters, NodeRetreiver& nodeRetreiver)
		: m_pWriters(std::move(pWriters))
		, m_nodeRetreiver(nodeRetreiver)
		, m_running(true)
		, m_workerThread(&MessageSender::workerThreadFunc, this)
	{}

	MessageSender::~MessageSender() {
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_running = false;
		}
		m_condVar.notify_one();
		m_workerThread.join();
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

	void MessageSender::enqueue(const std::shared_ptr<MessagePacket>& pPacket, const std::set<ProcessId>& recipients) {
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_buffer.emplace_back(pPacket, recipients);
		}
		m_condVar.notify_one();
	}

	void MessageSender::clearQueue() {
		std::lock_guard<std::mutex> guard(m_mutex);
		m_buffer.clear();
	}

	void MessageSender::workerThreadFunc() {
		BufferType buffer;
		while (m_running) {
			{
				std::unique_lock<std::mutex> lock(m_mutex);
				m_condVar.wait(lock, [this] {
					return !m_buffer.empty() || !m_running;
				});
				std::swap(buffer, m_buffer);

				if (!m_running) {
					return;
				}
			}

			BufferType unsentMessages;
			net::PeerConnectResult peerConnectResult;
			for (const auto& pair : buffer) {
				std::set<ProcessId> skippedNodes;
				const auto& pPacket = pair.first;
				const auto& recipients = pair.second;
				getPacketIos(recipients);
				std::vector<thread::future<bool>> completionStatusFutures;
				for (const auto& recipient : recipients) {
					auto iter = m_packetIoPairs.find(recipient);
					if (iter != m_packetIoPairs.end()) {
						auto node = getNodeName(m_nodeRetreiver, recipient);
						CATAPULT_LOG(trace) << "[DBRB] sending " << *pPacket << " to " << node;
						auto pPromise = std::make_shared<thread::promise<bool>>();
						completionStatusFutures.push_back(pPromise->get_future());
						iter->second->io()->write(ionet::PacketPayload(pPacket), [pThisWeak = weak_from_this(), node, pPacket, pPromise, recipient](ionet::SocketOperationCode code) {
							if (code != ionet::SocketOperationCode::Success) {
								CATAPULT_LOG(error) << "[DBRB] sending " << *pPacket << " to " << node << " completed with " << code;
								auto pThis = pThisWeak.lock();
								if (pThis) {
									pThis->m_packetIoPairs.erase(recipient);
									pThis->m_nodeRetreiver.removeNode(recipient);
								}
							}
							pPromise->set_value(true);
						});
					} else {
						skippedNodes.emplace(recipient);
					}
				}

				if (!completionStatusFutures.empty()) {
					thread::when_all(std::move(completionStatusFutures)).then([](auto&& completedFutures) {
						return thread::get_all_ignore_exceptional(completedFutures.get());
					}).get();
				}

				if (!skippedNodes.empty())
					unsentMessages.emplace_back(pPacket, skippedNodes);
			}

			buffer.clear();

			{
				std::lock_guard<std::mutex> guard(m_mutex);
				unsentMessages.insert(unsentMessages.end(), std::make_move_iterator(m_buffer.begin()), std::make_move_iterator(m_buffer.end()));
				m_buffer.clear();
				std::swap(unsentMessages, m_buffer);
			}
		}
	}
}}