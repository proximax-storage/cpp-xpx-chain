/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoSdaOfferGroupCacheStorage.h"
#include "src/mappers/SdaOfferGroupEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/exchange_sda/src/cache/SdaOfferGroupCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

    namespace {
        struct SdaOfferGroupCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::SdaOfferGroupCacheDescriptor> {
            static constexpr const char* Collection_Name = "sdaoffergroups";
            static constexpr const char* Id_Property_Name = "sdaoffergroups.groupHash";

            static auto MapToMongoId(const KeyType& key) {
                return mappers::ToBinary(key);
            }

            static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
                return plugins::ToDbModel(entry);
            }

            static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
                cache.insert(ToSdaOfferGroupEntry(document));
            }
        };
    }

    DEFINE_MONGO_FLAT_CACHE_STORAGE(SdaOfferGroup, SdaOfferGroupCacheTraits)
}}}