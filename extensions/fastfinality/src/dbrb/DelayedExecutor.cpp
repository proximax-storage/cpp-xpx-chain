/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DelayedExecutor.h"
#include "catapult/exceptions.h"
#include "catapult/thread/IoThreadPool.h"
#include <boost/asio/system_timer.hpp>
#include <set>
#include <utility>

namespace catapult { namespace dbrb {

	namespace {
		class DefaultDelayedExecutor : public DelayedExecutor, public std::enable_shared_from_this<DefaultDelayedExecutor> {
		public:
			explicit DefaultDelayedExecutor(std::shared_ptr<thread::IoThreadPool> pPool)
				: m_pPool(std::move(pPool))
			{}

			~DefaultDelayedExecutor() override {
				cancel();
			}

		public:
			void execute(uint64_t delayMillis, const action& callback) override {
				auto pTimer = std::make_shared<boost::asio::system_timer>(m_pPool->ioContext());
				pTimer->expires_after(std::chrono::milliseconds(delayMillis));
				pTimer->async_wait([pThisWeak = weak_from_this(), pTimerWeak = std::weak_ptr<boost::asio::system_timer>(pTimer), callback](const boost::system::error_code& ec) {
					auto pThis = pThisWeak.lock();
					if (!pThis)
						return;

					auto pTimer = pTimerWeak.lock();
					if (pTimer) {
						std::lock_guard<std::mutex> guard(pThis->m_mutex);
						pThis->m_timers.erase(pTimer);
					}

					if (ec) {
						if (ec == boost::asio::error::operation_aborted)
							return;

						CATAPULT_THROW_EXCEPTION(boost::system::system_error(ec));
					}

					callback();
				});

				{
					std::lock_guard<std::mutex> guard(m_mutex);
					m_timers.emplace(pTimer);
				}
			}

			void cancel() override {
				std::lock_guard<std::mutex> guard(m_mutex);
				for (const auto& pTimer : m_timers)
					pTimer->cancel();
			}

		private:
			std::shared_ptr<thread::IoThreadPool> m_pPool;
			std::set<std::shared_ptr<boost::asio::system_timer>> m_timers;
			std::mutex m_mutex;
		};
	}

	std::shared_ptr<DelayedExecutor> CreateDelayedExecutor(const std::shared_ptr<thread::IoThreadPool>& pPool) {
		return std::make_shared<DefaultDelayedExecutor>(pPool);
	}
}}