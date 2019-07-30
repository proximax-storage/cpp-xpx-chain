/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/storages/MongoCatapultConfigCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/model/Address.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "mongo/tests/test/MongoTestUtils.h"
#include "plugins/txes/config/tests/test/CatapultConfigTestUtils.h"
#include "tests/test/CatapultConfigMapperTestUtils.h"
#include "tests/TestHarness.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoCatapultConfigCacheStorageTests

	namespace {
		struct CatapultConfigCacheTraits {
			using CacheType = cache::CatapultConfigCache;
			using ModelType = state::CatapultConfigEntry;

			static constexpr auto Collection_Name = "catapultConfigs";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoCatapultConfigCacheStorage;

			static cache::CatapultCache CreateCache(const model::BlockChainConfiguration& config) {
				return test::CatapultConfigCacheFactory::Create(config);
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				auto height = Height{id};

				state::CatapultConfigEntry entry(height, "aaa", "bbb");

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& catapultConfigCacheDelta = delta.sub<cache::CatapultConfigCache>();
				catapultConfigCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& catapultConfigCacheDelta = delta.sub<cache::CatapultConfigCache>();
				catapultConfigCacheDelta.remove(entry.height());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				entry.setBlockChainConfig("ccc");
				entry.setSupportedEntityVersions("ddd");

				// update cache
				auto& catapultConfigCacheDelta = delta.sub<cache::CatapultConfigCache>();
				auto& entryFromCache = catapultConfigCacheDelta.find(entry.height()).get();
				entryFromCache.setBlockChainConfig("ccc");
				entryFromCache.setSupportedEntityVersions("ddd");
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "catapultConfig.height" << mappers::ToInt64(entry.height()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				test::AssertEqualCatapultConfigData(entry, view["catapultConfig"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(CatapultConfigCacheTraits,)
}}}
