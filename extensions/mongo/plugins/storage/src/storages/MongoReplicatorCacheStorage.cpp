/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoReplicatorCacheStorage.h"
#include "src/mappers/ReplicatorEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/storage//src/cache/ReplicatorCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct ReplicatorCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::ReplicatorCacheDescriptor> {
			static constexpr const char* Collection_Name = "replicators";
			static constexpr const char* Id_Property_Name = "replicator.key";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			// TODO: Is it safe to remove networkIdentifier?
			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry, entry.key());
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToReplicatorEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(Replicator, ReplicatorCacheTraits)
}}}
