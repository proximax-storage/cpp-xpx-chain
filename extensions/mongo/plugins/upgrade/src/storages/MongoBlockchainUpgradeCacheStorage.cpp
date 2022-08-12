/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MongoBlockchainUpgradeCacheStorage.h"
#include "src/mappers/BlockchainUpgradeEntryMapper.h"
#include "mongo/src/storages/MongoCacheStorage.h"
#include "plugins/txes/upgrade/src/cache/BlockchainUpgradeCache.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

	namespace {
		struct BlockchainUpgradeCacheTraits : public storages::BasicMongoCacheStorageTraits<cache::BlockchainUpgradeCacheDescriptor> {
			static constexpr const char* Collection_Name = "blockchainUpgrades";
			static constexpr const char* Id_Property_Name = "blockchainUpgrade.height";

			static auto MapToMongoId(const KeyType& key) {
				return mappers::ToInt64(key);
			}

			static auto MapToMongoDocument(const ModelType& entry, model::NetworkIdentifier) {
				return plugins::ToDbModel(entry);
			}

			static void Insert(CacheDeltaType& cache, const bsoncxx::document::view& document) {
				cache.insert(ToBlockchainUpgradeEntry(document));
			}
		};
	}

	DEFINE_MONGO_FLAT_CACHE_STORAGE(BlockchainUpgrade, BlockchainUpgradeCacheTraits)
}}}
