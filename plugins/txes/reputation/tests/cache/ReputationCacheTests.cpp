/**
*** Copyright (c) 2018-present,
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

		struct ReputationEntryModificationPolicy {
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
		auto key = test::GenerateRandomData<Key_Size>();

		// - insert single account key
		{

			auto delta = cache.createDelta();
			delta->insert(state::ReputationEntry(key));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1u, cache.createView()->size());

		// Act:
		{
			auto delta = cache.createDelta();
			auto& entry = delta->find(key).get();
			entry.incrementPositiveInteractions();
			entry.incrementNegativeInteractions();
			cache.commit();
		}

		// Assert:
		auto view = cache.createView();
		const auto& entry = view->find(key).get();
		EXPECT_EQ(1u, entry.positiveInteractions().unwrap());
		EXPECT_EQ(1u, entry.negativeInteractions().unwrap());
	}

	// endregion
}}
