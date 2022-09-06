/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "src/cache/GlobalStoreCache.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"
#include "tests/test/GlobalStoreTestUtils.h"

namespace catapult { namespace cache {

#define TEST_CLASS GlobalStoreCacheTests

	// region mixin traits based tests

	namespace {
		struct GlobalStoreCacheMixinTraits {
			class CacheType : public GlobalStoreCache {
			public:
				CacheType() : GlobalStoreCache(CacheConfiguration(), test::CreateGlobalStoreConfigHolder())
				{}
			};

			using IdType = Hash256;
			using ValueType = state::GlobalEntry;

			static uint8_t GetRawId(const IdType& id) {
				return id[0];
			}

			static IdType GetId(const ValueType& entry) {
				return entry.GetKey();
			}

			static IdType MakeId(uint8_t id) {
				return IdType{ { id } };
			}

			static ValueType CreateWithId(uint8_t id) {
				auto generatedVal = test::GenerateRandomArray<30>();
				std::vector<uint8_t> randomValue(generatedVal.begin(), generatedVal.end());
				return state::GlobalEntry(MakeId(id), randomValue);
			}
		};

		struct GlobalStoreCacheDeltaModificationPolicy : public test:: DeltaInsertModificationPolicy {
			static void Modify(GlobalStoreCacheDelta& delta, const state::GlobalEntry& entry) {
				auto& entryFromCache = delta.find(entry.GetKey()).get();
				entryFromCache.Ref().push_back(15);
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(GlobalStoreCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(GlobalStoreCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(GlobalStoreCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(GlobalStoreCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(GlobalStoreCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(GlobalStoreCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(GlobalStoreCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(GlobalStoreCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(GlobalStoreCacheMixinTraits, GlobalStoreCacheDeltaModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(GlobalStoreCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, CacheWrappersExposeNetworkIdentifier) {
		// Arrange:
		auto networkIdentifier = static_cast<model::NetworkIdentifier>(18);
		GlobalStoreCache cache(CacheConfiguration(), test::CreateGlobalStoreConfigHolder(networkIdentifier));

		// Act + Assert:
		EXPECT_EQ(networkIdentifier, cache.createView(Height())->networkIdentifier());
		EXPECT_EQ(networkIdentifier, cache.createDelta(Height())->networkIdentifier());
		EXPECT_EQ(networkIdentifier, cache.createDetachedDelta(Height()).tryLock()->networkIdentifier());
	}

	// endregion
}}
