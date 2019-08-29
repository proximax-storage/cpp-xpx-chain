/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/storages/MongoReputationCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/model/Address.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "mongo/tests/test/MongoTestUtils.h"
#include "plugins/txes/contract/tests/test/ReputationCacheTestUtils.h"
#include "tests/test/ReputationMapperTestUtils.h"
#include "tests/TestHarness.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoReputationCacheStorageTests

	namespace {
		struct ReputationCacheTraits {
			using CacheType = cache::ReputationCache;
			using ModelType = state::ReputationEntry;

			static constexpr auto Collection_Name = "reputations";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoReputationCacheStorage;

			static cache::CatapultCache CreateCache(const model::NetworkConfiguration& config) {
				return test::ReputationCacheFactory::Create(config);
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				auto key = Key();
				std::memcpy(key.data(), &id, sizeof(id));

				state::ReputationEntry entry(key);
				entry.setPositiveInteractions(Reputation{12u});
				entry.setNegativeInteractions(Reputation{34u});

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& reputationCacheDelta = delta.sub<cache::ReputationCache>();
				reputationCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& reputationCacheDelta = delta.sub<cache::ReputationCache>();
				reputationCacheDelta.remove(entry.key());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				entry.setPositiveInteractions(Reputation{24u});

				// update cache
				auto& reputationCacheDelta = delta.sub<cache::ReputationCache>();
				auto& entryFromCache = reputationCacheDelta.find(entry.key()).get();
				entryFromCache.setPositiveInteractions(Reputation{24u});
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "reputation.account" << mappers::ToBinary(entry.key()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				auto address = model::PublicKeyToAddress(entry.key(), Network_Id);
				test::AssertEqualReputationData(entry, address, view["reputation"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(ReputationCacheTraits,)
}}}
