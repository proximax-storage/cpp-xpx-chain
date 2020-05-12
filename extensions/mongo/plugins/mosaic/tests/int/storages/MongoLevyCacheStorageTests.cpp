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

#include "src/storages/MongoLevyCacheStorage.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MongoFlatCacheStorageTests.h"
#include "mongo/tests/test/MongoTestUtils.h"
#include "plugins/txes/mosaic/tests/test/LevyTestUtils.h"
#include "plugins/txes/mosaic/src/state/LevyEntry.h"
#include "tests/test/LevyEntryMapperTestUtils.h"
#include "tests/TestHarness.h"

using namespace bsoncxx::builder::stream;

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MongoLevyCacheStorageTests
			
	namespace {
		struct LevyCacheTraits {
			using CacheType = cache::LevyCache;
			using ModelType = state::LevyEntry;
			
			static constexpr auto Collection_Name = "levy";
			static constexpr auto Network_Id = static_cast<model::NetworkIdentifier>(0x5A);
			static constexpr auto CreateCacheStorage = CreateMongoLevyCacheStorage;
			
			static cache::CatapultCache CreateCache() {
				return test::LevyCacheFactory::Create();
			}
			
			static ModelType GenerateRandomElement(uint32_t id) {
				return state::LevyEntry(MosaicId(id), test::CreateValidMosaicLevy());
			}
			
			static void Add(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& LevyCacheDelta = delta.sub<cache::LevyCache>();
				LevyCacheDelta.insert(entry);
			}
			
			static void Remove(cache::CatapultCacheDelta& delta, const ModelType& entry) {
				auto& LevyCacheDelta = delta.sub<cache::LevyCache>();
				LevyCacheDelta.remove(entry.mosaicId());
			}
			
			static void Mutate(cache::CatapultCacheDelta& delta, ModelType& entry) {
				// update expected
				
				// update cache
				auto& LevyCacheDelta = delta.sub<cache::LevyCache>();
				auto& entryFromCache = LevyCacheDelta.find(entry.mosaicId()).get();
				entryFromCache.levy()->Fee = Amount(1000);
			}
			
			static auto GetFindFilter(const ModelType& entry) {
				return document() << "levy.mosaicId" << mappers::ToInt64(entry.mosaicId()) << finalize;
			}
			
			static void AssertEqual(const ModelType& entry, const bsoncxx::document::view& view) {
				test::AssertEqualLevyData(entry, view["levy"].get_document().view());
			}
		};
	}
	
	DEFINE_FLAT_CACHE_STORAGE_TESTS(LevyCacheTraits,)
}}}
