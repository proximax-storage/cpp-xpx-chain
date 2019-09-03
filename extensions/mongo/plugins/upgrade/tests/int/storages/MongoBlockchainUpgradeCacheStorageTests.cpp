/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/storages/MongoBlockchainUpgradeCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/model/Address.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "mongo/tests/test/MongoTestUtils.h"
#include "plugins/txes/upgrade/tests/test/BlockchainUpgradeTestUtils.h"
#include "tests/test/BlockchainUpgradeMapperTestUtils.h"
#include "tests/TestHarness.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoBlockchainUpgradeCacheStorageTests

	namespace {
		struct BlockchainUpgradeCacheTraits {
			using CacheType = cache::BlockchainUpgradeCache;
			using ModelType = state::BlockchainUpgradeEntry;

			static constexpr auto Collection_Name = "blockchainUpgrades";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoBlockchainUpgradeCacheStorage;

			static cache::CatapultCache CreateCache() {
				return test::BlockchainUpgradeCacheFactory::Create();
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				auto height = Height{id};

				state::BlockchainUpgradeEntry entry(height, BlockchainVersion{7});

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& blockchainUpgradeCacheDelta = delta.sub<cache::BlockchainUpgradeCache>();
				blockchainUpgradeCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& blockchainUpgradeCacheDelta = delta.sub<cache::BlockchainUpgradeCache>();
				blockchainUpgradeCacheDelta.remove(entry.height());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				entry.setBlockchainVersion(BlockchainVersion{15});

				// update cache
				auto& blockchainUpgradeCacheDelta = delta.sub<cache::BlockchainUpgradeCache>();
				auto& entryFromCache = blockchainUpgradeCacheDelta.find(entry.height()).get();
				entryFromCache.setBlockchainVersion(BlockchainVersion{15});
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "blockchainUpgrade.height" << mappers::ToInt64(entry.height()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				test::AssertEqualBlockchainUpgradeData(entry, view["blockchainUpgrade"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(BlockchainUpgradeCacheTraits,)
}}}
