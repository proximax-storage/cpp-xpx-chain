/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Messages.h"
#include <condition_variable>
#include <future>
#include <thread>

namespace catapult { namespace net { class PacketWriters; }}

namespace catapult { namespace dbrb {

	class MessageSender : public utils::NonCopyable {
	public:
		explicit MessageSender(std::shared_ptr<net::PacketWriters> pWriters);
		~MessageSender();

	public:
		void enqueue(const std::shared_ptr<MessagePacket>& pPacket, const std::set<ProcessId>& recipients);

	private:
		void workerThreadFunc();

	private:
		using BufferType = std::vector<std::pair<std::shared_ptr<MessagePacket>, std::set<ProcessId>>>;
		BufferType m_buffer;
		std::shared_ptr<net::PacketWriters> m_pWriters;
		std::mutex m_mutex;
		std::condition_variable m_condVar;
		bool m_running;
		std::thread m_workerThread;
	};
}}