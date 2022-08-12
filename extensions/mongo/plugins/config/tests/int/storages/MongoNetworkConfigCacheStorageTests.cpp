/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/storages/MongoNetworkConfigCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "plugins/txes/config/tests/test/NetworkConfigTestUtils.h"
#include "tests/test/NetworkConfigMapperTestUtils.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoNetworkConfigCacheStorageTests

	namespace {
		struct NetworkConfigCacheTraits {
			using CacheType = cache::NetworkConfigCache;
			using ModelType = state::NetworkConfigEntry;

			static constexpr auto Collection_Name = "networkConfigs";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoNetworkConfigCacheStorage;

			static cache::CatapultCache CreateCache() {
				return test::NetworkConfigCacheFactory::Create();
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				auto height = Height{id};

				state::NetworkConfigEntry entry(height, test::networkConfig(), test::supportedVersions());

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& networkConfigCacheDelta = delta.sub<cache::NetworkConfigCache>();
				networkConfigCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& networkConfigCacheDelta = delta.sub<cache::NetworkConfigCache>();
				networkConfigCacheDelta.remove(entry.height());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				entry.setBlockChainConfig(test::networkConfig());
				entry.setSupportedEntityVersions(test::supportedVersions());

				// update cache
				auto& networkConfigCacheDelta = delta.sub<cache::NetworkConfigCache>();
				auto& entryFromCache = networkConfigCacheDelta.find(entry.height()).get();
				entryFromCache.setBlockChainConfig(test::networkConfig());
				entryFromCache.setSupportedEntityVersions(test::supportedVersions());
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "networkConfig.height" << mappers::ToInt64(entry.height()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				test::AssertEqualNetworkConfigData(entry, view["networkConfig"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(NetworkConfigCacheTraits,)
}}}
