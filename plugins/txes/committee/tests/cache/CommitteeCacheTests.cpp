/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/CommitteeTestUtils.h"
#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"

namespace catapult { namespace cache {

#define TEST_CLASS CommitteeCacheTests

	// region mixin traits based tests

	namespace {
		struct CommitteeCacheMixinTraits {
			class CacheType : public CommitteeCache {
			public:
				CacheType()
					: CommitteeCache(CacheConfiguration(), std::make_shared<cache::CommitteeAccountCollector>(), config::CreateMockConfigurationHolder())
				{}

				CacheType(std::shared_ptr<cache::CommitteeAccountCollector> pAccountCollector)
					: CommitteeCache(CacheConfiguration(), pAccountCollector, config::CreateMockConfigurationHolder())
				{}
			};

			using IdType = Key;
			using ValueType = state::CommitteeEntry;

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
				return test::CreateCommitteeEntry(MakeId(id));
			}
		};

		struct CommitteeEntryModificationPolicy : public test::DeltaRemoveInsertModificationPolicy {
			static void Modify(CommitteeCacheDelta& delta, const state::CommitteeEntry& entry) {
				auto& entryFromCache = delta.find(entry.key()).get();
				entryFromCache.setEffectiveBalance(Importance(100));
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(CommitteeCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(CommitteeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(CommitteeCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_ACCESSOR_TESTS(CommitteeCacheMixinTraits, ViewAccessor, MutableAccessor, _ViewMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(CommitteeCacheMixinTraits, ViewAccessor, ConstAccessor, _ViewConst)
	DEFINE_CACHE_ACCESSOR_TESTS(CommitteeCacheMixinTraits, DeltaAccessor, MutableAccessor, _DeltaMutable)
	DEFINE_CACHE_ACCESSOR_TESTS(CommitteeCacheMixinTraits, DeltaAccessor, ConstAccessor, _DeltaConst)

	DEFINE_CACHE_MUTATION_TESTS(CommitteeCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_DELTA_ELEMENTS_MIXIN_CUSTOM_TESTS(CommitteeCacheMixinTraits, CommitteeEntryModificationPolicy, _Delta)

	DEFINE_CACHE_BASIC_TESTS(CommitteeCacheMixinTraits,)

	// endregion

	// region custom tests

	TEST(TEST_CLASS, InteractionValuesCanBeChangedLater) {
		// Arrange:
		CommitteeCacheMixinTraits::CacheType cache;
		auto key = test::GenerateRandomByteArray<Key>();

		// - insert single account key
		{
			auto delta = cache.createDelta(Height{1});
			delta->insert(test::CreateCommitteeEntry(key));
			cache.commit();
		}

		// Sanity:
		EXPECT_EQ(1u, cache.createView(Height{1})->size());

		// Act:
		{
			auto delta = cache.createDelta(Height{1});
			auto& entry = delta->find(key).get();
			entry.setEffectiveBalance(Importance(100));
			cache.commit();
		}

		// Assert:
		auto view = cache.createView(Height{0});
		const auto& entry = view->find(key).get();
		EXPECT_EQ(Importance(100), entry.effectiveBalance());
	}

	TEST(TEST_CLASS, CommitUpdatesAccountCollector) {
		// Arrange:
		auto pAccountCollector = std::make_shared<cache::CommitteeAccountCollector>();
		CommitteeCacheMixinTraits::CacheType cache(pAccountCollector);
		auto entry = test::CreateCommitteeEntry();
		auto key1 = test::GenerateRandomByteArray<Key>();
		auto key2 = test::GenerateRandomByteArray<Key>();

		// Act:
		{
			auto delta = cache.createDelta(Height(1));
			delta->insert(entry);
			delta->insert(test::CreateCommitteeEntry(key1));
			delta->insert(test::CreateCommitteeEntry(key2));
			delta->remove(key1);
			cache.commit();
		}

		{
			auto delta = cache.createDelta(Height(1));
			delta->remove(key2);
			cache.commit();
		}

		// Assert:
		const auto& accounts = pAccountCollector->accounts();
		EXPECT_EQ(1, accounts.size());
		test::AssertEqualAccountData(entry.data(), accounts.at(entry.key()));
	}

	// endregion
}}
