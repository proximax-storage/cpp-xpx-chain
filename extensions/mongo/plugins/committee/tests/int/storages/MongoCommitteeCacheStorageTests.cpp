/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/storages/MongoCommitteeCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "plugins/txes/committee/tests/test/CommitteeTestUtils.h"
#include "tests/test/CommitteeMapperTestUtils.h"
#include "catapult/model/Address.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoCommitteeCacheStorageTests

	namespace {
		struct CommitteeCacheTraits {
			using CacheType = cache::CommitteeCache;
			using ModelType = state::CommitteeEntry;

			static constexpr auto Collection_Name = "harvesters";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoCommitteeCacheStorage;

			static cache::CatapultCache CreateCache() {
				return test::CommitteeCacheFactory::Create();
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				Key key;
				std::memcpy(key.data(), &id, sizeof(id));

				auto entry = test::CreateCommitteeEntry(key);

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& committeeCacheDelta = delta.sub<cache::CommitteeCache>();
				committeeCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& committeeCacheDelta = delta.sub<cache::CommitteeCache>();
				committeeCacheDelta.remove(entry.key());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				entry.setEffectiveBalance(Importance(1000));

				// update cache
				auto& committeeCacheDelta = delta.sub<cache::CommitteeCache>();
				auto& entryFromCache = committeeCacheDelta.find(entry.key()).get();
				entryFromCache.setEffectiveBalance(Importance(1000));
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "harvester.key" << mappers::ToBinary(entry.key()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				auto address = model::PublicKeyToAddress(entry.key(), Network_Id);
				test::AssertEqualCommitteeData(entry, address, view["harvester"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(CommitteeCacheTraits,)
}}}
