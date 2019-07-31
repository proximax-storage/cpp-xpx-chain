/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/ReputationCache.h"
#include "tests/test/ReputationTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS ReputationCacheTests

	// region mixin traits based tests

	namespace {
		struct ReputationCacheMixinTraits {
			class CacheType : public ReputationCache {
			public:
				CacheType() : ReputationCache(CacheConfiguration())
				{}
			};

			using IdType = Key;
			using ValueType = state::ReputationEntry;

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
				return state::ReputationEntry(MakeId(id));
			}
		};

		struct ReputationEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(ReputationCacheDelta& delta, const state::ReputationEntry& entry) {
				auto& entryFromCache = delta.find(entry.key()).get();
				entryFromCache.incrementPositiveInteractions();
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(ReputationCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(ReputationCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(ReputationCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(ReputationCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(ReputationCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(ReputationCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(ReputationCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(ReputationCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(ReputationCacheMixinTraits, ReputationEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(ReputationCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		ReputationCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();

		// - insert single account key
		{

			auto delta = cache.createDelta(Height{0});
			delta->insert(state::ReputationEntry(key));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1u, cache.createView(Height{0})->size());

		// Act:
		{
			auto delta = cache.createDelta(Height{0});
			auto& entry = delta->find(key).get();
			entry.incrementPositiveInteractions();
			entry.incrementNegativeInteractions();
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height{0});
		const auto& entry = view->find(key).get();
		EXPECT_EQ(1u, entry.positiveInteractions().unwrap());
		EXPECT_EQ(1u, entry.negativeInteractions().unwrap());
	}

	// endregion
}}
