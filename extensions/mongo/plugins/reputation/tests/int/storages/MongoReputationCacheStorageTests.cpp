/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/storages/MongoReputationCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "catapult/model/Address.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "mongo/tests/test/MongoTestUtils.h"
#include "plugins/txes/reputation/tests/test/ReputationCacheTestUtils.h"
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

			static cache::CatapultCache CreateCache() {
				return test::ReputationCacheFactory::Create();
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
				auto& entryFromCache = reputationCacheDelta.get(entry.key());
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
