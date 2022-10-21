/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoSdaExchangeCacheStorage.h"
#include "src/mappers/SdaExchangeEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/exchange_sda/src/cache/SdaExchangeCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

    namespace {
        struct SdaExchangeCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::SdaExchangeCacheDescriptor> {
            static constexpr const char* Collection_Name = "exchangesda";
            static constexpr const char* Id_Property_Name = "exchangesda.owner";

            static auto MapToMongoId(const KeyType& key) {
                return mappers::ToBinary(key);
            }

            static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
                return plugins::ToDbModel(entry, model::PublicKeyToAddress(entry.owner(), networkIdentifier));
            }

            static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
                cache.insert(ToSdaExchangeEntry(document));
            }
        };
    }

    DEFINE_MONGO_FLAT_CACHE_STORAGE(SdaExchange, SdaExchangeCacheTraits)
}}}