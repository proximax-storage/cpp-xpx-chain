/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/storages/MongoSdaOfferGroupCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "plugins/txes/exchange_sda/tests/test/SdaExchangeTestUtils.h"
#include "tests/test/SdaExchangeMapperTestUtils.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoSdaOfferGroupCacheStorageTests

    namespace {
        struct SdaOfferGroupCacheTraits {
            using CacheType = cache::SdaOfferGroupCache;
            using ModelType = state::SdaOfferGroupEntry;

            static constexpr auto Collection_Name = "sdaoffergroups";
            static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
            static constexpr auto CreateCacheStorage = CreateMongoSdaOfferGroupCacheStorage;

            static cache::CatapultCache CreateCache() {
                return test::SdaOfferGroupCacheFactory::Create();
            }

            static ModelType GenerateRandomElement(uint32_t id) {
                Hash256 groupHash;
                std::memcpy(groupHash.data(), &id, sizeof(id));

                auto entry = test::CreateSdaOfferGroupEntry(10, groupHash);

                return entry;
            }

            static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
                auto& offerCacheDelta = delta.sub<cache::SdaOfferGroupCache>();
                offerCacheDelta.insert(entry);
            }

            static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
                auto& offerCacheDelta = delta.sub<cache::SdaOfferGroupCache>();
                offerCacheDelta.remove(entry.groupHash());
            }

            static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
                // update expected
                auto groupHash = test::GenerateRandomByteArray<Hash256>();
                auto offer = test::GenerateSdaOfferBasicInfo();
                entry.sdaOfferGroup().emplace_back(offer);

                // update cache
                auto& offerCacheDelta = delta.sub<cache::SdaOfferGroupCache>();
                auto& entryFromCache = offerCacheDelta.find(entry.groupHash()).get();
                entryFromCache.sdaOfferGroup().emplace_back(offer);
            }

            static auto GetFindFilter(const ModelType& entry) {
                return document() << "sdaoffergroups.groupHash" << mappers::ToBinary(entry.groupHash()) << finalize;
            }

            static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
                test::AssertEqualSdaOfferGroupData(entry, view["sdaoffergroups"].get_document().view());
            }
        };
    }

    DEFINE_FLAT_CACHE_STORAGE_TESTS(SdaOfferGroupCacheTraits,)
}}}