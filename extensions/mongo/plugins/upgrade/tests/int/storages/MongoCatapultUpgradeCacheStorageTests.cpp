/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/storages/MongoCatapultUpgradeCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/model/Address.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "mongo/tests/test/MongoTestUtils.h"
#include "plugins/txes/upgrade/tests/test/CatapultUpgradeTestUtils.h"
#include "tests/test/CatapultUpgradeMapperTestUtils.h"
#include "tests/TestHarness.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoCatapultUpgradeCacheStorageTests

	namespace {
		struct CatapultUpgradeCacheTraits {
			using CacheType = cache::CatapultUpgradeCache;
			using ModelType = state::CatapultUpgradeEntry;

			static constexpr auto Collection_Name = "catapultUpgrades";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoCatapultUpgradeCacheStorage;

			static cache::CatapultCache CreateCache(const model::BlockChainConfiguration& config) {
				return test::CatapultUpgradeCacheFactory::Create(config);
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				auto height = Height{id};

				state::CatapultUpgradeEntry entry(height, CatapultVersion{7});

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& catapultUpgradeCacheDelta = delta.sub<cache::CatapultUpgradeCache>();
				catapultUpgradeCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& catapultUpgradeCacheDelta = delta.sub<cache::CatapultUpgradeCache>();
				catapultUpgradeCacheDelta.remove(entry.height());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				entry.setCatapultVersion(CatapultVersion{15});

				// update cache
				auto& catapultUpgradeCacheDelta = delta.sub<cache::CatapultUpgradeCache>();
				auto& entryFromCache = catapultUpgradeCacheDelta.find(entry.height()).get();
				entryFromCache.setCatapultVersion(CatapultVersion{15});
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "catapultUpgrade.height" << mappers::ToInt64(entry.height()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				test::AssertEqualCatapultUpgradeData(entry, view["catapultUpgrade"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(CatapultUpgradeCacheTraits,)
}}}
