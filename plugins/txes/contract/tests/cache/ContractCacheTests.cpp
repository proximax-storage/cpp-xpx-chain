/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/ContractCache.h"
#include "tests/test/ContractCacheTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS ContractCacheTests

	// region mixin traits based tests

	namespace {
		struct ContractCacheMixinTraits {
			class CacheType : public ContractCache {
			public:
				CacheType() : ContractCache(CacheConfiguration())
				{}
			};

			using IdType = Key;
			using ValueType = state::ContractEntry;

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
				return state::ContractEntry(MakeId(id));
			}
		};

		struct ContractEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(ContractCacheDelta& delta, const state::ContractEntry& entry) {
				auto& entryFromCache = delta.find(entry.key()).get();
				entryFromCache.setStart(entryFromCache.start() + Height(1));
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(ContractCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(ContractCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(ContractCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(ContractCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(ContractCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(ContractCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(ContractCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(ContractCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(ContractCacheMixinTraits, ContractEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(ContractCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		ContractCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();

		// - insert single account key
		{

			auto delta = cache.createDelta();
			delta->insert(state::ContractEntry(key));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1u, cache.createView()->size());

		// Act:
		{
			auto delta = cache.createDelta();
			auto& entry = delta->find(key).get();
			entry.setDuration(BlockDuration(20));
			entry.setStart(Height(12));
			cache.commit();
		}

		// Assert:
		auto view = cache.createView();
		const auto& entry = view->find(key).get();
		EXPECT_EQ(20u, entry.duration().unwrap());
		EXPECT_EQ(12u, entry.start().unwrap());
	}

	// endregion
}}
