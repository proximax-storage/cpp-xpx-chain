/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/BlockchainUpgradeCache.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS BlockchainUpgradeCacheTests

	// region mixin traits based tests

	namespace {
		struct BlockchainUpgradeCacheMixinTraits {
			class CacheType : public BlockchainUpgradeCache {
			public:
				CacheType() : BlockchainUpgradeCache(CacheConfiguration())
				{}
			};

			using IdType = Height;
			using ValueType = state::BlockchainUpgradeEntry;

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
				return state::BlockchainUpgradeEntry(MakeId(id));
			}
		};

		struct BlockchainUpgradeEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(BlockchainUpgradeCacheDelta& delta, const state::BlockchainUpgradeEntry& entry) {
				auto& entryFromCache = delta.find(entry.height()).get();
				entryFromCache.setBlockchainVersion(BlockchainVersion{entryFromCache.blockChainVersion().unwrap() + 1u});
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(BlockchainUpgradeCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(BlockchainUpgradeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(BlockchainUpgradeCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(BlockchainUpgradeCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(BlockchainUpgradeCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(BlockchainUpgradeCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(BlockchainUpgradeCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(BlockchainUpgradeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(BlockchainUpgradeCacheMixinTraits, BlockchainUpgradeEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(BlockchainUpgradeCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		BlockchainUpgradeCacheMixinTraits::CacheType cache;
		auto key = Height{2};

		// - insert single account key
		{

			auto delta = cache.createDelta(Height{1});
			delta->insert(state::BlockchainUpgradeEntry(key));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1, cache.createView(Height{1})->size());

		// Act:
		{
			auto delta = cache.createDelta(Height{1});
			auto& entry = delta->find(key).get();
			entry.setBlockchainVersion(BlockchainVersion{10});
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height{0});
		const auto& entry = view->find(key).get();
		EXPECT_EQ(BlockchainVersion{10}, entry.blockChainVersion());
	}

	// endregion
}}
