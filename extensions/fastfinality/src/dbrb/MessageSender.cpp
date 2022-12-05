/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MessageSender.h"
#include "catapult/net/PacketWriters.h"
#include "catapult/utils/TimeSpan.h"

namespace catapult { namespace dbrb {

	namespace {
		constexpr auto Default_Timeout = utils::TimeSpan::FromMinutes(1);

		constexpr utils::LogLevel MapToLogLevel(net::PeerConnectCode connectCode) {
			if (connectCode == net::PeerConnectCode::Accepted)
				return utils::LogLevel::Info;
			else
				return utils::LogLevel::Warning;
		}

		net::PeerConnectResult Connect(net::PacketWriters& writers, const ProcessId& node) {
			auto pPromise = std::make_shared<std::promise<net::PeerConnectResult>>();
			writers.connect(node, [pPromise, node](const net::PeerConnectResult& result) {
				const auto& endPoint = node.endpoint();
				CATAPULT_LOG_LEVEL(MapToLogLevel(result.Code))
					<< "[DBRB] connection attempt to " << node << " @ " << endPoint.Host << " : " << endPoint.Port << " completed with " << result.Code;
				pPromise->set_value(result);
			});

			return pPromise->get_future().get();
		}

		ionet::NodePacketIoPair GetNodePacketIoPair(net::PacketWriters& writers, const ProcessId& node, net::PeerConnectResult& result) {
			auto nodePacketIoPair = writers.pickOne(Default_Timeout, node.identityKey());
			if (nodePacketIoPair.io())
				return nodePacketIoPair;

			result = Connect(writers, node);
			if (result.Code == net::PeerConnectCode::Accepted)
				return writers.pickOne(Default_Timeout, node.identityKey());

			return {};
		}
	}

	MessageSender::MessageSender(std::shared_ptr<net::PacketWriters> pWriters)
		: m_pWriters(std::move(pWriters))
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

	void MessageSender::enqueue(const std::shared_ptr<MessagePacket>& pPacket, const std::set<ProcessId>& recipients) {
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_buffer.emplace_back(pPacket, recipients);
		}
		m_condVar.notify_one();
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
				std::set<ProcessId> unprocessedNodes;
				const auto& pPacket = pair.first;
				const auto& recipients = pair.second;
				for (const auto& recipient : recipients) {
					auto nodePacketIoPair = GetNodePacketIoPair(*m_pWriters, recipient, peerConnectResult);
					if (nodePacketIoPair.io()) {
						auto pPromise = std::make_shared<std::promise<ionet::SocketOperationCode>>();
						nodePacketIoPair.io()->write(ionet::PacketPayload(pPacket), [pPacket, recipient, pPromise](ionet::SocketOperationCode code) {
							pPromise->set_value(code);
						});
						auto code = pPromise->get_future().get();
						if (code != ionet::SocketOperationCode::Success)
							CATAPULT_LOG(error) << "[DBRB] sending " << *pPacket << " to " << recipient << " completed with " << code;
					} else {
						if (peerConnectResult.Code == net::PeerConnectCode::Already_Connected)
							unprocessedNodes.emplace(recipient);
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
		}
	}
}}