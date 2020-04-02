/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/storages/MongoSuperContractCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "plugins/txes/supercontract/tests/test/SuperContractTestUtils.h"
#include "tests/test/SuperContractMapperTestUtils.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoSuperContractCacheStorageTests

	namespace {
		struct SuperContractCacheTraits {
			using CacheType = cache::SuperContractCache;
			using ModelType = state::SuperContractEntry;

			static constexpr auto Collection_Name = "supercontracts";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoSuperContractCacheStorage;

			static cache::CatapultCache CreateCache() {
				return test::SuperContractCacheFactory::Create();
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				Key key;
				std::memcpy(key.data(), &id, sizeof(id));

				auto entry = test::CreateSuperContractEntry(key);

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& offerCacheDelta = delta.sub<cache::SuperContractCache>();
				offerCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& offerCacheDelta = delta.sub<cache::SuperContractCache>();
				offerCacheDelta.remove(entry.key());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				entry.setStart(Height(10));

				// update cache
				auto& supercontractCacheDelta = delta.sub<cache::SuperContractCache>();
				auto& entryFromCache = supercontractCacheDelta.find(entry.key()).get();
				entryFromCache.setStart(Height(10));
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "supercontract.multisig" << mappers::ToBinary(entry.key()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				auto address = model::PublicKeyToAddress(entry.key(), Network_Id);
				test::AssertEqualMongoSuperContractData(entry, address, view["supercontract"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(SuperContractCacheTraits,)
}}}
