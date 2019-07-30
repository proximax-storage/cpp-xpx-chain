/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/CatapultUpgradeCache.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS CatapultUpgradeCacheTests

	// region mixin traits based tests

	namespace {
		struct CatapultUpgradeCacheMixinTraits {
			class CacheType : public CatapultUpgradeCache {
			public:
				CacheType() : CatapultUpgradeCache(CacheConfiguration())
				{}
			};

			using IdType = Height;
			using ValueType = state::CatapultUpgradeEntry;

			static uint8_t GetRawId(const IdType& id) {
				return utils::checked_cast<uint64_t, uint8_t>(id.unwrap());
			}

			static IdType GetId(const ValueType& entry) {
				return entry.height();
			}

			static IdType MakeId(uint8_t id) {
				return IdType{ id };
			}

			static ValueType CreateWithId(uint8_t id) {
				return state::CatapultUpgradeEntry(MakeId(id));
			}
		};

		struct CatapultUpgradeEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(CatapultUpgradeCacheDelta& delta, const state::CatapultUpgradeEntry& entry) {
				auto& entryFromCache = delta.find(entry.height()).get();
				entryFromCache.setCatapultVersion(CatapultVersion{entryFromCache.catapultVersion().unwrap() + 1u});
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(CatapultUpgradeCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(CatapultUpgradeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(CatapultUpgradeCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(CatapultUpgradeCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(CatapultUpgradeCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(CatapultUpgradeCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(CatapultUpgradeCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(CatapultUpgradeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(CatapultUpgradeCacheMixinTraits, CatapultUpgradeEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(CatapultUpgradeCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		CatapultUpgradeCacheMixinTraits::CacheType cache;
		auto key = Height{2};

		// - insert single account key
		{

			auto delta = cache.createDelta(Height{1});
			delta->insert(state::CatapultUpgradeEntry(key));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1, cache.createView(Height{1})->size());

		// Act:
		{
			auto delta = cache.createDelta(Height{1});
			auto& entry = delta->find(key).get();
			entry.setCatapultVersion(CatapultVersion{10});
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height{0});
		const auto& entry = view->find(key).get();
		EXPECT_EQ(CatapultVersion{10}, entry.catapultVersion());
	}

	// endregion
}}
