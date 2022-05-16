/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoPriorityQueueCacheStorage.h"
#include "src/mappers/PriorityQueueEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/storage/src/cache/PriorityQueueCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct PriorityQueueCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::PriorityQueueCacheDescriptor> {
			static constexpr const char* Collection_Name = "priorityQueues";
			static constexpr const char* Id_Property_Name = "priorityQueueDoc.queueKey";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry, entry.key());
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToPriorityQueueEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(PriorityQueue, PriorityQueueCacheTraits)
}}}
