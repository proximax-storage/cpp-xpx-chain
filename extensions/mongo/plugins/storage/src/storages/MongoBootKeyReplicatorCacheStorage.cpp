/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoBootKeyReplicatorCacheStorage.h"
#include "src/mappers/BootKeyReplicatorEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/storage//src/cache/BootKeyReplicatorCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct BootKeyReplicatorCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::BootKeyReplicatorCacheDescriptor> {
			static constexpr const char* Collection_Name = "bootKeyReplicators";
			static constexpr const char* Id_Property_Name = "bootKeyReplicator.nodeBootKey";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry, entry.nodeBootKey());
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToBootKeyReplicatorEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(BootKeyReplicator, BootKeyReplicatorCacheTraits)
}}}
