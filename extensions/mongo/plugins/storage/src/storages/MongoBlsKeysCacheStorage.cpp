/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoBlsKeysCacheStorage.h"
#include "src/mappers/BlsKeysEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/storage/src/cache/BlsKeysCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct BlsKeysCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::BlsKeysCacheDescriptor> {
			static constexpr const char* Collection_Name = "blsKeys";
			static constexpr const char* Id_Property_Name = "blsKeyDoc.blsKey";

			static auto MapToMongoId(const KeyType& blsKey) {
				return mappers::ToBinary(blsKey);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry, entry.blsKey());
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToBlsKeysEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(BlsKeys, BlsKeysCacheTraits)
}}}
