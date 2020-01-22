/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/cache/DownloadCache.h"
#include "tests/test/ServiceTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS DownloadCacheTests

	// region mixin traits based tests

	namespace {
		struct DownloadCacheMixinTraits {
			class CacheType : public DownloadCache {
			public:
				CacheType() : DownloadCache(CacheConfiguration(), config::CreateMockConfigurationHolder())
				{}
			};

			using IdType = Key;
			using ValueType = state::DownloadEntry;

			static uint8_t GetRawId(const IdType& id) {
				return id[0];
			}

			static IdType GetId(const ValueType& entry) {
				return entry.driveKey();
			}

			static IdType MakeId(uint8_t id) {
				return IdType{ { id } };
			}

			static ValueType CreateWithId(uint8_t id) {
				auto entry = test::CreateDownloadEntry(MakeId(id));
				entry.fileRecipients().emplace(test::GenerateRandomByteArray<Key>(), state::DownloadMap{});
				return entry;
			}
		};

		struct DownloadEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(DownloadCacheDelta& delta, const state::DownloadEntry& entry) {
				auto iter = delta.find(entry.driveKey());
				auto& entryFromCache = iter.get();
				entryFromCache.fileRecipients().clear();
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(DownloadCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(DownloadCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(DownloadCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(DownloadCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(DownloadCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(DownloadCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(DownloadCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(DownloadCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(DownloadCacheMixinTraits, DownloadEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(DownloadCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		DownloadCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();

		{
			auto delta = cache.createDelta(Height(1));
			auto entry = test::CreateDownloadEntry(key);
			delta->insert(entry);
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1, cache.createView(Height(1))->size());

		// Act:
		{
			auto delta = cache.createDelta(Height(1));
			auto& entry = delta->find(key).get();
			entry.fileRecipients().clear();
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height(0));
		const auto& entry = view->find(key).get();
		EXPECT_TRUE(entry.fileRecipients().empty());
	}

	// endregion
}}
