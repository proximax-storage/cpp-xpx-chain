/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/DriveCache.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS DriveCacheTests

	// region mixin traits based tests

	namespace {
		constexpr Height Initial_Start_Height(5);
		constexpr Height Expected_Start_Height(10);

		struct DriveCacheMixinTraits {
			class CacheType : public DriveCache {
			public:
				CacheType() : DriveCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
				{}
			};

			using IdType = Key;
			using ValueType = state::DriveEntry;

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
				auto entry = test::CreateDriveEntry(MakeId(id));
				entry.setStart(Initial_Start_Height);
				return entry;
			}
		};

		struct DriveEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(DriveCacheDelta& delta, const state::DriveEntry& entry) {
				auto iter = delta.find(entry.key());
				auto& entryFromCache = iter.get();
				entryFromCache.setStart(Expected_Start_Height);
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(DriveCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(DriveCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(DriveCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(DriveCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(DriveCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(DriveCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(DriveCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(DriveCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(DriveCacheMixinTraits, DriveEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(DriveCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		DriveCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();

		// - insert single account key
		{

			auto delta = cache.createDelta(Height(1));
			auto entry = test::CreateDriveEntry(key);
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
