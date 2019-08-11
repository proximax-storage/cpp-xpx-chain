/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/CatapultConfigCache.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS CatapultConfigCacheTests

	// region mixin traits based tests

	namespace {
		struct CatapultConfigCacheMixinTraits {
			class CacheType : public CatapultConfigCache {
			public:
				CacheType() : CatapultConfigCache(CacheConfiguration())
				{}
			};

			using IdType = Height;
			using ValueType = state::CatapultConfigEntry;

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
				return state::CatapultConfigEntry(MakeId(id));
			}
		};

		struct CatapultConfigEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(CatapultConfigCacheDelta& delta, const state::CatapultConfigEntry& entry) {
				auto& entryFromCache = delta.find(entry.height()).get();
				entryFromCache.setBlockChainConfig(entryFromCache.blockChainConfig() + "\nnewField = 1");
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(CatapultConfigCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(CatapultConfigCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(CatapultConfigCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(CatapultConfigCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(CatapultConfigCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(CatapultConfigCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(CatapultConfigCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(CatapultConfigCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(CatapultConfigCacheMixinTraits, CatapultConfigEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(CatapultConfigCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		CatapultConfigCacheMixinTraits::CacheType cache;
		auto key = Height{2};

		// - insert single account key
		{

			auto delta = cache.createDelta(Height{1});
			delta->insert(state::CatapultConfigEntry(key));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1u, cache.createView(Height{1})->size());

		// Act:
		{
			auto delta = cache.createDelta(Height{1});
			auto& entry = delta->find(key).get();
			entry.setBlockChainConfig("blockChainConfig");
			entry.setSupportedEntityVersions("supportedEntityVersions");
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height{0});
		const auto& entry = view->find(key).get();
		EXPECT_EQ("blockChainConfig", entry.blockChainConfig());
		EXPECT_EQ("supportedEntityVersions", entry.supportedEntityVersions());
	}

	// endregion
}}
