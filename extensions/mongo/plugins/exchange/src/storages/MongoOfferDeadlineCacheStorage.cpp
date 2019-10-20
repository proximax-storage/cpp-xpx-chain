/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoOfferDeadlineCacheStorage.h"
#include "src/mappers/OfferDeadlineEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/exchange/src/cache/OfferDeadlineCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct OfferDeadlineCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::OfferDeadlineCacheDescriptor> {
			static constexpr const char* Collection_Name = "offerDeadlines";
			static constexpr const char* Id_Property_Name = "offerDeadline.height";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToInt64(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToOfferDeadlineEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(OfferDeadline, OfferDeadlineCacheTraits)
}}}
