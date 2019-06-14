/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoCatapultConfigCacheStorage.h"
#include "src/mappers/CatapultConfigEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/config/src/cache/CatapultConfigCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct CatapultConfigCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::CatapultConfigCacheDescriptor> {
			static constexpr const char* Collection_Name = "catapultConfigs";
			static constexpr const char* Id_Property_Name = "catapultConfig.height";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToInt64(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToCatapultConfigEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(CatapultConfig, CatapultConfigCacheTraits)
}}}
