/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "src/state/QueueEntry.h"

namespace catapult::utils {

template<class TCache>
class QueueAdapter {
public:
	QueueAdapter(
		cache::QueueCache::CacheDeltaType& queueCache,
		const Key& queueKey,
		typename TCache::CacheDeltaType& cache)
			: m_cache(cache)
			, m_queueCache(queueCache)
			, m_queueKey(queueKey) {
		if (!m_queueCache.contains(m_queueKey)) {
			state::QueueEntry entry(m_queueKey);
			m_queueCache.insert(entry);
		}
	}

	bool isEmpty() {
		auto queueIter = m_queueCache.find(m_queueKey);
		const auto& queueEntry = queueIter.get();
		return queueEntry.getFirst() == Key();
	}

	Key front() {
		auto queueIter = m_queueCache.find(m_queueKey);
		const auto& queueEntry = queueIter.get();
		return queueEntry.getFirst();
	}

	void popFront() {
		auto queueIter = m_queueCache.find(m_queueKey);
		auto& queueEntry = queueIter.get();

		auto iter = m_cache.find(queueEntry.getFirst().array());
		auto& entry = iter.get();
		queueEntry.setFirst(entry.getQueueNext().array());

		if (entry.getQueueNext() != Key()) {
			// Previous of the first MUST be "null"
			m_cache.find(queueEntry.getFirst().array()).get().setQueuePrevious(Key());
		}
		else {
			// There is only one element in the Queue. So after removal there are no elements at all
			queueEntry.setFirst(Key());
			queueEntry.setLast(Key());
		}
	}

	void pushBack(const Key& key) {
		auto queueIter = m_queueCache.find(m_queueKey);
		auto& queueEntry = queueIter.get();

		auto iter = m_cache.find(key.array());
		auto& entry = iter.get();
		entry.setQueueNext(Key());

		if (isEmpty()) {
			queueEntry.setFirst(key);
			queueEntry.setLast(key);
			entry.setQueuePrevious(Key());
		}
		else {
			auto iter2 = m_cache.find(queueEntry.getLast().array());
			auto& lastEntry = iter2.get();
			queueEntry.setLast(key);
			lastEntry.setQueueNext(key);
			entry.setQueuePrevious(lastEntry.entryKey());
		}
	}

	void remove(const Key& key) {
		auto queueIter = m_queueCache.find(m_queueKey);
		auto& queueEntry = queueIter.get();

		auto iter = m_cache.find(key);
		auto& entry = iter.get();

		if (entry.getQueuePrevious() != Key()) {
			auto previousDriveIter = m_cache.find(entry.getQueuePrevious());
			auto& previousDriveEntry = previousDriveIter.get();
			previousDriveEntry.setQueueNext(entry.getQueueNext());
		}
		else {
			// Previous link is "null" so the entry is first in the queue
			queueEntry.setFirst(entry.getQueueNext());
			if (entry.getQueueNext() != Key()) {
				auto nextDriveIter = m_cache.find(entry.getQueueNext());
				auto& nextDriveEntry = nextDriveIter.get();
				nextDriveEntry.setQueuePrevious(Key());
			}
		}

		if (entry.getQueueNext() != Key()) {
			auto nextDriveIter = m_cache.find(entry.getQueueNext());
			auto& nextDriveEntry = nextDriveIter.get();
			nextDriveEntry.setQueuePrevious(entry.getQueuePrevious());
		}
		else {
			queueEntry.setLast(entry.getQueuePrevious());
			if (entry.getQueuePrevious() != Key()) {
				auto previousDriveIter = m_cache.find(entry.getQueuePrevious());
				auto& previousDriveEntry = previousDriveIter.get();
				previousDriveEntry.setQueueNext(Key());
			}
		}
	}

private:
	cache::QueueCache::CacheDeltaType& m_queueCache;
	Key m_queueKey;

	typename TCache::CacheDeltaType& m_cache;
};
}