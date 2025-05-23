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
#include <thread>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>

namespace catapult { namespace utils {

	/// Exception class that is thrown when reader reentrancy is detected.
	class reader_reentrancy_error : public catapult_runtime_error {
	public:
		using catapult_runtime_error::catapult_runtime_error;
	};

	/// A reentrancy check reader notification policy.
	class ReentrancyCheckReaderNotificationPolicy {
	public:
		/// A reader was acquried by the current thread.
		void readerAcquired() {
			executeSynchronized([this](auto id) {
				if (m_threadIds.cend() != m_threadIds.find(id))
					CATAPULT_THROW_AND_LOG_1(reader_reentrancy_error, "reader reentrancy detected", id);

				m_threadIds.insert(id);
			});
		}

		/// A reader was released by the current thread.
		void readerReleased() {
			executeSynchronized([this](auto id) {
				m_threadIds.erase(id);
			});
		}

	private:
		template<typename TAction>
		void executeSynchronized(TAction action) {
			auto id = std::this_thread::get_id();
			std::unique_lock lock(m_mutex);

			action(id);
		}

	private:
		std::shared_mutex m_mutex;
		std::unordered_set<std::thread::id> m_threadIds;
	};
}}
