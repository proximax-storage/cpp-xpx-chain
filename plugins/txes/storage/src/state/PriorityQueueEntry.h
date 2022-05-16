/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/types.h"
#include "catapult/exceptions.h"
#include "catapult/model/Mosaic.h"
#include "catapult/utils/ArraySet.h"
#include "catapult/utils/IntegerMath.h"
#include <queue>

namespace catapult { namespace state {

	static const Key DrivePriorityQueueKey = { { 1 } };

	struct PriorityPair {
		catapult::Key Key;
		double Priority = 0.0;

		friend bool operator< (const PriorityPair& a, const PriorityPair& b) {
			return a.Priority == b.Priority ? a.Key < b.Key : a.Priority < b.Priority;
		}
	};

	// Mixin for storing priority queue details.
	class PriorityQueueMixin {
	public:
		PriorityQueueMixin()
		{}

	public:
		/// Gets underlying priority queue.
		const std::priority_queue<PriorityPair>& priorityQueue() const {
			return m_priorityQueue;
		}

		/// Gets underlying priority queue.
		std::priority_queue<PriorityPair>& priorityQueue() {
			return m_priorityQueue;
		}

		/// Inserts a (\a key, \a priority) pair, removing all previously existing pairs with specified \a key.
		/// Returns the number of removed pairs.
		size_t set(const Key& key, const double& priority) {
			const size_t removedCount = removeWithCount(key);
			m_priorityQueue.push( {key, priority} );

			return removedCount;
		}

		/// Removes all priority pairs with specified \a key.
		/// Returns the number of removed pairs.
		size_t remove(const Key& key) {
			return removeWithCount(key);
		}

		/// Removes all priority pairs with specified \a key. If any were removed, inserts one (\a key, \a newPriority) pair.
		/// Returns the number of removed pairs.
		size_t update(const Key& key, const double& newPriority) {
			const size_t removedCount = removeWithCount(key);
			if (removedCount > 0)
				m_priorityQueue.push( {key, newPriority} );

			return removedCount;
		}

	private:
		/// Rebuilds \a m_priorityQueue, excluding all priority pairs with specified \a key.
		/// Returns the number of removed pairs.
		size_t removeWithCount(const Key& key) {
			size_t removedCount = 0;

			std::priority_queue<PriorityPair> newQueue;
			while (!m_priorityQueue.empty()) {
				auto priorityPair = m_priorityQueue.top();
				m_priorityQueue.pop();

				if (priorityPair.Key == key)
					removedCount++;
				else
					newQueue.push(std::move(priorityPair));
			}
			m_priorityQueue = std::move(newQueue);

			return removedCount;
		}

	private:
		std::priority_queue<PriorityPair> m_priorityQueue;
	};

	// Priority queue entry.
	class PriorityQueueEntry : public PriorityQueueMixin {
	public:
		// Creates a drive entry around \a key.
		explicit PriorityQueueEntry(const Key& key): m_key(key), m_version(1)
		{}

	public:

		void setVersion(VersionType version) {
			m_version = version;
		}

		VersionType version() const {
			return m_version;
		}

		// Gets the priority queue key.
		const Key& key() const {
			return m_key;
		}

	private:
		Key m_key;
		VersionType m_version;
	};
}}
