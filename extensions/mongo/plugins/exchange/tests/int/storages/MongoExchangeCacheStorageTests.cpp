/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/storages/MongoExchangeCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "plugins/txes/exchange/tests/test/ExchangeTestUtils.h"
#include "tests/test/ExchangeMapperTestUtils.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoExchangeCacheStorageTests

	namespace {
		struct ExchangeCacheTraits {
			using CacheType = cache::ExchangeCache;
			using ModelType = state::ExchangeEntry;

			static constexpr auto Collection_Name = "exchanges";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoExchangeCacheStorage;

			static cache::CatapultCache CreateCache() {
				return test::ExchangeCacheFactory::Create();
			}

			static ModelType GenerateRandomElement(uint32_t id) {
				Key key;
				std::memcpy(key.data(), &id, sizeof(id));

				auto entry = test::CreateExchangeEntry(10, 10, key);

				return entry;
			}

			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& offerCacheDelta = delta.sub<cache::ExchangeCache>();
				offerCacheDelta.insert(entry);
			}

			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& offerCacheDelta = delta.sub<cache::ExchangeCache>();
				offerCacheDelta.remove(entry.owner());
			}

			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				auto mosaicId = test::GenerateRandomValue<MosaicId>();
				auto offer = test::GenerateOffer();
				entry.sellOffers().emplace(mosaicId, state::SellOffer{offer});

				// update cache
				auto& offerCacheDelta = delta.sub<cache::ExchangeCache>();
				auto& entryFromCache = offerCacheDelta.find(entry.owner()).get();
				entryFromCache.sellOffers().emplace(mosaicId, state::SellOffer{offer});
			}

			static auto GetFindFilter(const ModelType& entry) {
				return document() << "exchange.owner" << mappers::ToBinary(entry.owner()) << finalize;
			}

			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				test::AssertEqualExchangeData(entry, view["exchange"].get_document().view());
			}
		};
	}

	DEFINE_FLAT_CACHE_STORAGE_TESTS(ExchangeCacheTraits,)
}}}
