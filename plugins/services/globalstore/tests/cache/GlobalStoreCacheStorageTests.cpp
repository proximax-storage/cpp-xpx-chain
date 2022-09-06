/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "src/cache/GlobalStoreCacheStorage.h"
#include "src/cache/GlobalStoreCache.h"
#include "tests/test/GlobalStoreTestUtils.h"
#include "tests/test/cache/CacheStorageTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

	namespace {
		struct GlobalStoreCacheStorageTraits {
			using StorageType = GlobalStoreCacheStorage;
			class CacheType : public GlobalStoreCache {
			public:
				CacheType() : GlobalStoreCache(CacheConfiguration(), test::CreateGlobalStoreConfigHolder())
				{}
			};

			static auto CreateId(uint8_t id) {
				return Hash256{ { id } };
			}

			static auto CreateValue(const Hash256& key) {

				auto generatedVal = test::GenerateRandomArray<30>();
				std::vector<uint8_t> randomValue(generatedVal.begin(), generatedVal.end());
				state::GlobalEntry entry(key, randomValue);
				return entry;
			}

			static void AssertEqual(const state::GlobalEntry& lhs, const state::GlobalEntry& rhs) {
				test::AssertEqual(lhs, rhs);
			}
		};
	}

	DEFINE_BASIC_INSERT_REMOVE_CACHE_STORAGE_TESTS(GlobalStoreCacheStorageTests, GlobalStoreCacheStorageTraits)
}}
