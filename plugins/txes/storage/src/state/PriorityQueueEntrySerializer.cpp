/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PriorityQueueEntrySerializer.h"
#include "catapult/io/PodIoUtils.h"
#include "catapult/utils/Casting.h"

namespace catapult { namespace state {

	namespace {

		void SavePriorityQueue(io::OutputStream& output, std::priority_queue<state::PriorityPair> priorityQueue) {
			io::Write16(output, priorityQueue.size());
			while (!priorityQueue.empty()) {
				const auto pair = priorityQueue.top();
				priorityQueue.pop();

				io::Write(output, pair.Key);
				io::WriteDouble(output, pair.Priority);
			}
		}

		void LoadPriorityQueue(io::InputStream& input, std::priority_queue<state::PriorityPair>& priorityQueue) {
			auto pairCount = io::Read16(input);
			while (pairCount--) {
				Key key;
				io::Read(input, key);
				const auto priority = io::ReadDouble(input);
				priorityQueue.push( {key, priority} );
			}
		}

	}

	void PriorityQueueEntrySerializer::Save(const PriorityQueueEntry& priorityQueueEntry, io::OutputStream& output) {
		io::Write32(output, priorityQueueEntry.version());
		io::Write(output, priorityQueueEntry.key());

		SavePriorityQueue(output, priorityQueueEntry.priorityQueue());
	}

	PriorityQueueEntry PriorityQueueEntrySerializer::Load(io::InputStream& input) {
		VersionType version = io::Read32(input);
		if (version > 1)
			CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of PriorityQueueEntry", version);

		Key queueKey;
		input.read(queueKey);
		state::PriorityQueueEntry entry(queueKey);
		entry.setVersion(version);

		LoadPriorityQueue(input, entry.priorityQueue());

		return entry;
	}
}}
