/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PriorityQueueEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include <queue>

using namespace catapult::mongo::mappers;

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	namespace {

		void StreamPriorityQueue(bson_stream::document& builder, std::priority_queue<state::PriorityPair> priorityQueue) {
			auto array = builder << "priorityQueue" << bson_stream::open_array;
			while (!priorityQueue.empty()) {
				const auto pair = priorityQueue.top();
				priorityQueue.pop();

				bson_stream::document pairBuilder;
				pairBuilder
						<< "key" << ToBinary(pair.Key)
						<< "priority" << static_cast<double>(pair.Priority);
				array << pairBuilder;
			}
			array << bson_stream::close_array;
		}

	}

	bsoncxx::document::value ToDbModel(const state::PriorityQueueEntry& entry) {
		bson_stream::document builder;
		auto doc = builder << "priorityQueueDoc" << bson_stream::open_document
						   << "queueKey" << ToBinary(entry.key())
						   << "version" << static_cast<int32_t>(entry.version());

		StreamPriorityQueue(builder, entry.priorityQueue());

		return doc
				<< bson_stream::close_document
				<< bson_stream::finalize;
	}

	// endregion

	// region ToModel

	namespace {

		void ReadPriorityQueue(std::priority_queue<state::PriorityPair>& priorityQueue, const bsoncxx::array::view& dbPriorityQueue) {
			for (const auto& dbPair : dbPriorityQueue) {
				auto doc = dbPair.get_document().view();

				Key key;
				DbBinaryToModelArray(key, doc["key"].get_binary());

				const auto priority = static_cast<double>(dbPair["priority"].get_double());

				priorityQueue.push( {key, priority} );
			}
		}

	}

	state::PriorityQueueEntry ToPriorityQueueEntry(const bsoncxx::document::view& document) {

		auto dbPriorityQueueEntry = document["priorityQueueDoc"];

		Key queueKey;
		DbBinaryToModelArray(queueKey, dbPriorityQueueEntry["queueKey"].get_binary());
		state::PriorityQueueEntry entry(queueKey);

		entry.setVersion(static_cast<VersionType>(dbPriorityQueueEntry["version"].get_int32()));

		ReadPriorityQueue(entry.priorityQueue(), dbPriorityQueueEntry["priorityQueue"].get_array().value);

		return entry;
	}

	// endregion
}}}
