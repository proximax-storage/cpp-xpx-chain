/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoExchangeCacheStorage.h"
#include "src/mappers/ExchangeEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/exchange/src/cache/ExchangeCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct ExchangeCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::ExchangeCacheDescriptor> {
			static constexpr const char* Collection_Name = "exchanges";
			static constexpr const char* Id_Property_Name = "exchange.owner";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToBinary(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToExchangeEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(Exchange, ExchangeCacheTraits)
}}}
