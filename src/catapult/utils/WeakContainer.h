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
#include "catapult/functions.h"
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace catapult { namespace utils {

	/// A container of weak_ptr<T> pointing to closable items.
	template<typename T>
	class WeakContainer {
	public:
		/// Creates an empty container.
		WeakContainer() : WeakContainer([](const auto&) {})
		{}

		/// Creates an empty container with a custom close function (\a close).
		WeakContainer(const consumer<T&>& close) : m_close(close)
		{}

	public:
		/// Gets the number of items in the container and removes all previously deleted items.
		size_t size() const {
			std::unique_lock lock(m_mutex);
			const_cast<WeakContainer*>(this)->pruneInternal();
			return m_entries.size();
		}

		/// Adds \a pEntry to this container and removes all previously deleted items.
		void insert(const std::weak_ptr<T>& pEntry) {
			std::unique_lock lock(m_mutex);
			pruneInternal();
			m_entries.push_back(pEntry);
		}

		/// Closes and removes all items in this container.
		void clear() {
			std::unique_lock lock(m_mutex);
			for (const auto& pEntry : m_entries) {
				auto pSharedEntry = pEntry.lock();
				if (!pSharedEntry)
					continue;

				m_close(*pSharedEntry);
			}

			m_entries.clear();
		}

	private:
		void pruneInternal() {
			m_entries.remove_if([](const auto& pExistingEntry) { return !pExistingEntry.lock(); });
		}

	private:
		consumer<T&> m_close;
		std::list<std::weak_ptr<T>> m_entries;
		mutable std::shared_mutex m_mutex;
	};
}}
