/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoBuyOfferCacheStorage.h"
#include "src/mappers/OfferEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/exchange/src/cache/BuyOfferCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct BuyOfferCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::BuyOfferCacheDescriptor> {
			static constexpr const char* Collection_Name = "buyOffers";
			static constexpr const char* Id_Property_Name = "buyOffer.transactionHash";

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

	DEFINE_MONGO_FLAT_CACHE_STORAGE(BuyOffer, BuyOfferCacheTraits)
}}}
