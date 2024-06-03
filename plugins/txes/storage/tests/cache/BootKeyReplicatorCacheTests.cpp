/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/BootKeyReplicatorCache.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"

namespace catapult { namespace cache {

#define TEST_CLASS BootKeyReplicatorCacheTests

	// region mixin traits based tests

	namespace {
		struct BootKeyReplicatorCacheMixinTraits {
			class CacheType : public BootKeyReplicatorCache {
			public:
				CacheType() : BootKeyReplicatorCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
				{}
			};

			using IdType = Key;
			using ValueType = state::BootKeyReplicatorEntry;

			static uint8_t GetRawId(const IdType& id) {
				return id[0];
			}

			static IdType GetId(const ValueType& entry) {
				return entry.nodeBootKey();
			}

			static IdType MakeId(uint8_t id) {
				return IdType{{ id }};
			}

			static ValueType CreateWithId(uint8_t id) {
				return state::BootKeyReplicatorEntry(MakeId(id), IdType());
			}
		};

		struct BootKeyReplicatorEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(BootKeyReplicatorCacheDelta& delta, const state::BootKeyReplicatorEntry& entry) {
				auto iter = delta.find(entry.nodeBootKey());
				auto& entryFromCache = iter.get();
				entryFromCache.setReplicatorKey(test::GenerateRandomByteArray<Key>());
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(BootKeyReplicatorCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(BootKeyReplicatorCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(BootKeyReplicatorCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(BootKeyReplicatorCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(BootKeyReplicatorCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(BootKeyReplicatorCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(BootKeyReplicatorCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(BootKeyReplicatorCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(BootKeyReplicatorCacheMixinTraits, BootKeyReplicatorEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(BootKeyReplicatorCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		BootKeyReplicatorCacheMixinTraits::CacheType cache;
		auto nodeBootKey = test::GenerateRandomByteArray<Key>();
		auto replicatorKey1 = test::GenerateRandomByteArray<Key>();
		auto replicatorKey2 = test::GenerateRandomByteArray<Key>();

		// - insert single account key
		{
			auto delta = cache.createDelta(Height{1});
			delta->insert(state::BootKeyReplicatorEntry(nodeBootKey, replicatorKey1));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1, cache.createView(Height{1})->size());

		// Act:
		{
			auto delta = cache.createDelta(Height{1});
			auto& entry = delta->find(nodeBootKey).get();
			entry.setReplicatorKey(replicatorKey2);
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height{0});
		const auto& entry = view->find(nodeBootKey).get();
		EXPECT_EQ(replicatorKey2, entry.replicatorKey());
	}

	// endregion
}}
