/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "src/storages/MongoSdaExchangeCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "plugins/txes/exchange_sda/tests/test/SdaExchangeTestUtils.h"
#include "tests/test/SdaExchangeMapperTestUtils.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoSdaExchangeCacheStorageTests

    namespace {
        struct SdaExchangeCacheTraits {
            using CacheType = cache::SdaExchangeCache;
            using ModelType = state::SdaExchangeEntry;

            static constexpr auto Collection_Name = "exchangesda";
            static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
            static constexpr auto CreateCacheStorage = CreateMongoSdaExchangeCacheStorage;

            static cache::CatapultCache CreateCache() {
                return test::SdaExchangeCacheFactory::Create();
            }

            static ModelType GenerateRandomElement(uint32_t id) {
                Key key;
                std::memcpy(key.data(), &id, sizeof(id));

                auto entry = test::CreateSdaExchangeEntry(10, 10, key);

                return entry;
            }

            static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
                auto& offerCacheDelta = delta.sub<cache::SdaExchangeCache>();
                offerCacheDelta.insert(entry);
            }

            static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
                auto& offerCacheDelta = delta.sub<cache::SdaExchangeCache>();
                offerCacheDelta.remove(entry.owner());
            }

            static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
                // update expected
                auto mosaicGiveId = test::GenerateRandomValue<MosaicId>();
                auto mosaicGetId = test::GenerateRandomValue<MosaicId>();
                state::MosaicsPair pair{mosaicGiveId, mosaicGetId};
                auto offer = test::GenerateSdaOfferBalance();
                entry.sdaOfferBalances().emplace(pair, offer);

                // update cache
                auto& offerCacheDelta = delta.sub<cache::SdaExchangeCache>();
                auto& entryFromCache = offerCacheDelta.find(entry.owner()).get();
                entryFromCache.sdaOfferBalances().emplace(pair, offer);
            }

            static auto GetFindFilter(const ModelType& entry) {
                return document() << "exchangesda.owner" << mappers::ToBinary(entry.owner()) << finalize;
            }

            static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
                auto address = model::PublicKeyToAddress(entry.owner(), Network_Id);
                test::AssertEqualSdaExchangeData(entry, address, view["exchangesda"].get_document().view());
            }
        };
    }

    DEFINE_FLAT_CACHE_STORAGE_TESTS(SdaExchangeCacheTraits,)
}}}
