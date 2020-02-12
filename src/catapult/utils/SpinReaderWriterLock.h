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
#include <boost/thread/shared_mutex.hpp>

namespace catapult { namespace utils {

	// minutes before we declare waiting for lock as deadlock
	const int deadlock_threshold_minutes = 10;

	template<typename TReaderNotificationPolicy>
	class BasicSpinReaderWriterLock : private TReaderNotificationPolicy {
	public:
		// region WriterLockGuard
		/// RAII writer lock guard.
		class WriterLockGuard {
		public:

			/// \note This constructor is used when writer is created by promotion.
			WriterLockGuard(boost::upgrade_mutex* mutex, const action& resetFunc)
					: m_lock(*mutex, boost::chrono::minutes (deadlock_threshold_minutes))
					, m_resetFunc(resetFunc) {

				if(!m_lock.owns_lock()) {
					CATAPULT_THROW_RUNTIME_ERROR("Deadlock occur waiting for writer lock");
				}
			}

			WriterLockGuard(boost::upgrade_mutex* mutex)
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
			boost::unique_lock<boost::upgrade_mutex> m_lock;
			action m_resetFunc;
		};

		// endregion

		// region ReaderLockGuard

		/// RAII reader lock guard.
		class ReaderLockGuard {
		public:
			ReaderLockGuard(boost::upgrade_mutex* mutex)
					: m_lock(*mutex,
							boost::chrono::minutes (deadlock_threshold_minutes)) {

				if(!m_lock.owns_lock()) {
					CATAPULT_THROW_RUNTIME_ERROR("Deadlock occur waiting for reader lock");
				}
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
				m_lock = boost::shared_lock<boost::upgrade_mutex>(*m_lock.mutex());
			}

		private:
			boost::shared_lock<boost::upgrade_mutex> m_lock;
		};
		// endregion

		// region unique writer lock
		typedef boost::upgrade_to_unique_lock< boost::upgrade_mutex> UniqueWriteLock;
		// endregion

		// region UpgradableReaderLockGuard

		// boost/thread/shared_mutex.hpp does not support upgrade timed lock(try_lock_upgrade_for),
		// the alternative version in boost/thread/v2/shared_mutex.hpp cannot be used as the file has conflict with
		// boost/thread/shared_mutex.hpp and both files define 'boost::shared_mutex' explicitly.
		// boost/thread/shared_mutex.hpp is part of boost/thread.hpp which is potentially by other files.
		class UpgradableReaderLockGuard {
		public:
			UpgradableReaderLockGuard(boost::upgrade_mutex* mutex)
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
			boost::upgrade_lock<boost::upgrade_mutex> m_upgradeLock;
		};
		// endregion

	public:
		/// Creates an unlocked lock.
		BasicSpinReaderWriterLock() = default;

	public:

		inline ReaderLockGuard acquireReader() {
			return ReaderLockGuard(&m_mutex);
		}

		// \n wait parameter set to true if caller wants to wait
		// else throw an exception immediately
		inline UpgradableReaderLockGuard acquireUpgradableLock(bool wait = true) {
			if( !wait && !upgradeLockAllowed()) {
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
		// Instead we will try to obtain it and unlock immediately as we need scoped lock.
		bool upgradeLockAllowed() {
			if (m_mutex.try_lock_upgrade()) {
				m_mutex.unlock_upgrade();
				return true;
			}

			return false;
		}

	private:
		mutable boost::upgrade_mutex m_mutex;
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
