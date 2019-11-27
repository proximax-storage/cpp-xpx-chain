/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/storages/MongoDriveCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"
#include "tests/test/ServiceMapperTestUtils.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoDriveCacheStorageTests

	namespace {
		struct DriveCacheTraits {
			using CacheType = cache::DriveCache;
			using ModelType = state::DriveEntry;

			static constexpr auto Collection_Name = "drives";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoDriveCacheStorage;

			static cache::CatapultCache CreateCache() {
				return test::DriveCacheFactory::Create();
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				Key key;
				std::memcpy(key.data(), &id, sizeof(id));

				auto entry = test::CreateDriveEntry(key);

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& offerCacheDelta = delta.sub<cache::DriveCache>();
				offerCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& offerCacheDelta = delta.sub<cache::DriveCache>();
				offerCacheDelta.remove(entry.key());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				entry.setMinReplicators(10);

				// update cache
				auto& driveCacheDelta = delta.sub<cache::DriveCache>();
				auto& entryFromCache = driveCacheDelta.find(entry.key()).get();
				entryFromCache.setMinReplicators(10);
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "drive.multisig" << mappers::ToBinary(entry.key()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				auto address = model::PublicKeyToAddress(entry.key(), Network_Id);
				test::AssertEqualDriveData(entry, address, view["drive"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(DriveCacheTraits,)
}}}
