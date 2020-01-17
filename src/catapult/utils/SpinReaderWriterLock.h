/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "catapult/exceptions.h"
#include "catapult/functions.h"
#include <atomic>
#include <thread>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

namespace catapult { namespace utils {

	template<typename TReaderNotificationPolicy>
	class BasicSpinReaderWriterLock : private TReaderNotificationPolicy {

	public:
		// region WriterLockGuard
		/// RAII writer lock guard.
		class WriterLockGuard {
		public:

			/// \note This constructor is used when writer is created by promotion.
			WriterLockGuard(boost::shared_mutex* mutex, const action& resetFunc)
					: m_lock(*mutex)
					, m_resetFunc(resetFunc) {
			}

			WriterLockGuard(boost::shared_mutex* mutex)
					: m_lock(*mutex)
					, m_resetFunc(0) {
			}

			~WriterLockGuard() {
				if(m_lock.owns_lock())
				{
					m_lock.unlock();

					if(m_resetFunc) {
						m_resetFunc();
					}
				}
			}

			/// Default move constructor.
			WriterLockGuard(WriterLockGuard&&) = default;

		private:
			boost::unique_lock<boost::shared_mutex> m_lock;
			action m_resetFunc;
		};

		// endregion

		// region ReaderLockGuard

		/// RAII reader lock guard.
		class ReaderLockGuard {
		public:
			ReaderLockGuard(boost::shared_mutex* mutex)
					: m_lock(*mutex){
			}

			/// Default move constructor.
			ReaderLockGuard(ReaderLockGuard&&) = default;

		public:

			/// Promotes this reader lock to a writer lock.
			WriterLockGuard promoteToWriter() {
				if( m_lock.owns_lock())	{
					m_lock.unlock();
				} else {
					CATAPULT_THROW_RUNTIME_ERROR("Unable to promoteToWriter, "
								  "reader does not own lock");
				}

				return WriterLockGuard(m_lock.mutex(), [&]() {
					restoreReaderLock();
				});
			}

		private:

			void restoreReaderLock() {
				m_lock = boost::shared_lock<boost::shared_mutex>(*m_lock.mutex());
			}

		private:
			boost::shared_lock<boost::shared_mutex> m_lock;
		};
		// endregion

		// region unique writer lock
		typedef boost::upgrade_to_unique_lock< boost::shared_mutex> UniqueWriteLock;
		// endregion

		// region UpgradableReaderLockGuard
		class UpgradableReaderLockGuard {
		public:
			UpgradableReaderLockGuard(boost::shared_mutex* mutex)
					: m_upgradeLock(*mutex){
			}

			/// Default move constructor.
			UpgradableReaderLockGuard(UpgradableReaderLockGuard&&) = default;

		public:

			/// Promotes this reader lock to a writer lock.
			UniqueWriteLock promoteToWriter() {
				return UniqueWriteLock(m_upgradeLock);
			}

		private:
			boost::upgrade_lock<boost::shared_mutex> m_upgradeLock;
		};
		// endregion

	public:
		/// Creates an unlocked lock.
		BasicSpinReaderWriterLock() = default;

	public:

		inline ReaderLockGuard acquireReader() {
			return ReaderLockGuard(&m_mutex);
		}

		inline UpgradableReaderLockGuard acquireUpgradableLock() {

			if( !upgradeLockAllowed()) {
				CATAPULT_THROW_RUNTIME_ERROR("Upgrade ownership is not available");
			}
			return UpgradableReaderLockGuard(&m_mutex);
		}
		inline WriterLockGuard acquireWriter() {

			return WriterLockGuard(&m_mutex);
		}

	private:
		// \note: boost has no method to check whether mutex upgrade ownership was already
		// owned by a thread, there is no exception or error thrown.
		// To check if upgrade ownershi is available we will try to obtain it.
		// if it is available, we immediately unlock since we need a scoped lock instead
		bool upgradeLockAllowed() {

			if (m_mutex.try_lock_upgrade()) {
				m_mutex.unlock_upgrade();
				return true;
			}

			return false;
		}

	private:
		mutable boost::shared_mutex m_mutex;
	};

	/// No-op reader notification policy.
	struct NoOpReaderNotificationPolicy {
		/// Reader was acquried by the current thread.
		constexpr void readerAcquired()
		{}

		/// Reader was released by the current thread.
		constexpr void readerReleased()
		{}
	};
}}

#ifdef ENABLE_CATAPULT_DIAGNOSTICS
#include "ReentrancyCheckReaderNotificationPolicy.h"
#endif

namespace catapult { namespace utils {

#ifdef ENABLE_CATAPULT_DIAGNOSTICS
	using DefaultReaderNotificationPolicy = ReentrancyCheckReaderNotificationPolicy;
#else
	using DefaultReaderNotificationPolicy = NoOpReaderNotificationPolicy;
#endif

	/// Default reader writer lock.
	using SpinReaderWriterLock = BasicSpinReaderWriterLock<DefaultReaderNotificationPolicy>;
}}
