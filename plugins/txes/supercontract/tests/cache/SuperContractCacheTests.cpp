/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/SuperContractCache.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS SuperContractCacheTests

	// region mixin traits based tests

	namespace {
		constexpr Height Initial_Start_Height(5);
		constexpr Height Expected_Start_Height(10);

		struct SuperContractCacheMixinTraits {
			class CacheType : public SuperContractCache {
			public:
				CacheType() : SuperContractCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
				{}
			};

			using IdType = Key;
			using ValueType = state::SuperContractEntry;

			static uint8_t GetRawId(const IdType& id) {
				return id[0];
			}

			static IdType GetId(const ValueType& entry) {
				return entry.key();
			}

			static IdType MakeId(uint8_t id) {
				return IdType{ { id } };
			}

			static ValueType CreateWithId(uint8_t id) {
				auto entry = test::CreateSuperContractEntry(MakeId(id));
				entry.setStart(Initial_Start_Height);
				return entry;
			}
		};

		struct SuperContractEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(SuperContractCacheDelta& delta, const state::SuperContractEntry& entry) {
				auto iter = delta.find(entry.key());
				auto& entryFromCache = iter.get();
				entryFromCache.setStart(Expected_Start_Height);
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(SuperContractCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(SuperContractCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(SuperContractCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(SuperContractCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(SuperContractCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(SuperContractCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(SuperContractCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(SuperContractCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(SuperContractCacheMixinTraits, SuperContractEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(SuperContractCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		SuperContractCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();

		// - insert single account key
		{

			auto delta = cache.createDelta(Height(1));
			auto entry = test::CreateSuperContractEntry(key);
			entry.setStart(Initial_Start_Height);
			delta->insert(entry);
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1, cache.createView(Height(1))->size());

		// Act:
		{
			auto delta = cache.createDelta(Height(1));
			auto& entry = delta->find(key).get();
			entry.setStart(Expected_Start_Height);
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height(0));
		const auto& entry = view->find(key).get();
		EXPECT_EQ(Expected_Start_Height, entry.start());
	}

	// endregion
}}
