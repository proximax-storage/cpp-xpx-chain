/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoDbrbViewCacheStorage.h"
#include "src/mappers/DbrbProcessEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/dbrb/src/cache/DbrbViewCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct DbrbViewCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::DbrbViewCacheDescriptor> {
			static constexpr const char* Collection_Name = "dbrbProcesses";
			static constexpr const char* Id_Property_Name = "dbrbProcess.processId";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToDbrbProcessEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(DbrbView, DbrbViewCacheTraits)
}}}
