/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/utils/NonCopyable.h"
#include <condition_variable>
#include <future>
#include <thread>
#include <optional>
#include <vector>
#include <set>

namespace catapult { namespace dbrb {

	template<typename TElement>
	class AsyncMessageQueue : public utils::NonCopyable {
	protected:
		using BufferType = std::vector<TElement>;

	public:
		AsyncMessageQueue()
			: m_running(true)
			, m_workerThread(&AsyncMessageQueue::workerThreadFunc, this)
		{}

		virtual ~AsyncMessageQueue() {
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_running = false;
			}
			m_condVar.notify_one();
			m_workerThread.join();
		}

	public:
		template<typename... TArgs>
		void enqueue(TArgs&&... args) {
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				m_buffer.emplace_back(std::forward<TArgs>(args)...);
			}
			m_condVar.notify_one();
		}

		void clearQueue() {
			std::lock_guard<std::mutex> guard(m_mutex);
			m_buffer.clear();
		}

	protected:
		virtual void processBuffer(BufferType& buffer) = 0;

	private:
		void workerThreadFunc() {
			BufferType buffer;
			while (m_running) {
				{
					std::unique_lock<std::mutex> lock(m_mutex);
					m_condVar.wait(lock, [this] { return !m_buffer.empty() || !m_running; });
					if (!m_running)
						return;

					std::swap(buffer, m_buffer);
				}

				this->processBuffer(buffer);
			}
		}

	protected:
		BufferType m_buffer;
		mutable std::mutex m_mutex;
		std::condition_variable m_condVar;
		bool m_running;
		std::thread m_workerThread;
	};
}}