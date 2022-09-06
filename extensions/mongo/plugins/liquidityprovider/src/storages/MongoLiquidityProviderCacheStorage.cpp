/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoLiquidityProviderCacheStorage.h"
#include "src/mappers/LiquidityProviderEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/liquidityprovider/src/cache/LiquidityProviderCache.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct LiquidityProviderCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::LiquidityProviderCacheDescriptor> {
			static constexpr const char* Collection_Name = "liquidityProviders";
			static constexpr const char* Id_Property_Name = "liquidityProvider.mosaicId";

			static auto MapToMongoId(const KeyType& key) {
				return static_cast<int64_t>(key.unwrap());
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier networkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToLiquidityProviderEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(LiquidityProvider, LiquidityProviderCacheTraits)
}}}
