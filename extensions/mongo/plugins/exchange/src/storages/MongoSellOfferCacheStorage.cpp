/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoSellOfferCacheStorage.h"
#include "src/mappers/OfferEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/exchange/src/cache/SellOfferCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct SellOfferCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::SellOfferCacheDescriptor> {
			static constexpr const char* Collection_Name = "sellOffers";
			static constexpr const char* Id_Property_Name = "sellOffer.transactionHash";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToInt32(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToOfferEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(SellOffer, SellOfferCacheTraits)
}}}
