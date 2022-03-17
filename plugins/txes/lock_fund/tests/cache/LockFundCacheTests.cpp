/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/cache/CacheBasicTests.h"
#include "tests/test/cache/CacheMixinsTests.h"
#include "tests/test/cache/DeltaElementsMixinTests.h"
#include "tests/test/core/mocks/MockBlockchainConfigurationHolder.h"
#include "tests/test/LockFundTestUtils.h"

namespace catapult { namespace cache {

#define TEST_CLASS LockFundCacheTests

	// region mixin traits based tests

	namespace {
		constexpr auto Grace_Period_Duration = 7u;

		auto CreateConfigHolder() {
			auto pluginConfig = config::LockFundConfiguration::Uninitialized();
			pluginConfig.MaxMosaicsSize = 256;
			pluginConfig.MinRequestUnlockCooldown = BlockDuration(200000);
			auto networkConfig = model::NetworkConfiguration::Uninitialized();
			networkConfig.BlockGenerationTargetTime = utils::TimeSpan::FromHours(1);
			networkConfig.SetPluginConfiguration(pluginConfig);
			return config::CreateMockConfigurationHolder(networkConfig);
		}

		struct LockFundCacheMixinTraits {
			class CacheType : public cache::LockFundCache {
			public:
				CacheType() : cache::LockFundCache(CacheConfiguration(), test::CreateLockFundConfigHolder())
				{}
			};

			using IdType = Height;
			using ValueType = state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>;


			static IdType GetId(const ValueType& history) {
				return history.Identifier;
			}
			static Key GetId(const state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>& history) {
				return history.Identifier;
			}
			static uint8_t GetRawId(const IdType& id) {
				return static_cast<uint8_t>(id.unwrap());
			}

			static IdType MakeId(uint8_t id) {
				return IdType(id);
			}
			static ValueType CreateWithId(uint8_t id) {
				return test::GenerateRecordGroup<state::LockFundHeightIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(Height(id), 1);
			}
		};
	}

	DEFINE_CACHE_CONTAINS_TESTS(LockFundCacheMixinTraits, ViewAccessor, _View)
	DEFINE_CACHE_CONTAINS_TESTS(LockFundCacheMixinTraits, DeltaAccessor, _Delta)

	DEFINE_CACHE_ITERATION_TESTS(LockFundCacheMixinTraits, ViewAccessor, _View)

	DEFINE_CACHE_BASIC_TESTS(LockFundCacheMixinTraits,)

	// endregion

	// *** custom tests ***

#define LF_EMPTY_SET { {},{} }
#define LF_SET(HeightValues, KeyValues) { {HeightValues},{KeyValues} }


	namespace {

		static std::tuple<std::set<Height>, std::set<Key>> CollectIds(const std::tuple<std::unordered_set<const state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>*>, std::unordered_set<const state::LockFundRecordGroup<state::LockFundKeyIndexDescriptor>*>>& elements) {
			std::tuple<std::set<Height>, std::set<Key>> ids;
			for (const auto& pElement : std::get<0>(elements))
				std::get<0>(ids).insert(LockFundCacheMixinTraits::GetId(*pElement));
			for (const auto& pElement : std::get<1>(elements))
				std::get<1>(ids).insert(LockFundCacheMixinTraits::GetId(*pElement));
			return ids;
		}
		static void AssertMarkedElements(
				const cache::LockFundCacheDelta& delta,
				const std::tuple<std::set<Height>, std::set<Key>>& expectedAddedIds,
				const std::tuple<std::set<Height>, std::set<Key>>& expectedModifiedIds,
				const std::tuple<std::set<Height>, std::set<Key>>& expectedRemovedIds) {
			EXPECT_EQ(expectedAddedIds, CollectIds(delta.addedElements()));
			EXPECT_EQ(expectedModifiedIds, CollectIds(delta.modifiedElements()));
			EXPECT_EQ(expectedRemovedIds, CollectIds(delta.removedElements()));
		}

		static void AssertInitiallyNoElementsAreMarkedAsAddedOrModifiedOrRemoved() {
			// Arrange:
			LockFundCacheMixinTraits::CacheType cache;
			auto delta = cache.createDelta(Height{0});

			// Assert:
			AssertMarkedElements(*delta, LF_EMPTY_SET, LF_EMPTY_SET, LF_EMPTY_SET);
		}

		static void AssertAddedElementsAreMarkedAsAdded() {
			// Arrange:
			LockFundCacheMixinTraits::CacheType cache;
			auto delta = cache.createDelta(Height{0});

			auto record = LockFundCacheMixinTraits::CreateWithId(123);
			// Act:
			delta->insert(record);

			// Assert:
			AssertMarkedElements(*delta, LF_SET(LockFundCacheMixinTraits::MakeId(123), record.LockFundRecords.cbegin()->first), LF_EMPTY_SET, LF_EMPTY_SET);
		}

		static void AssertModifiedElementsAreMarkedAsModified() {
			// Arrange:
			LockFundCacheMixinTraits::CacheType cache;
			auto delta = cache.createDelta(Height{0});
			auto insertRecord = LockFundCacheMixinTraits::CreateWithId(123);
			delta->insert(insertRecord);
			cache.commit();

			// Act:
			auto &record = delta->find(LockFundCacheMixinTraits::MakeId(123)).get();
			record.LockFundRecords.clear(); // Remove if not needed.
			// Assert: if elements are immutable, no modifications are possible
			AssertMarkedElements(*delta, LF_EMPTY_SET, LF_SET(LockFundCacheMixinTraits::MakeId(123), insertRecord.LockFundRecords.cbegin()->first), LF_EMPTY_SET);
		}

		static void AssertRemovedElementsAreMarkedAsRemoved() {
			// Arrange:
			LockFundCacheMixinTraits::CacheType cache;
			auto delta = cache.createDelta(Height{0});
			auto insertRecord = LockFundCacheMixinTraits::CreateWithId(123);
			delta->insert(insertRecord);
			cache.commit();

			// Act:
			delta->remove(LockFundCacheMixinTraits::MakeId(123));

			// Assert:
			AssertMarkedElements(*delta, LF_EMPTY_SET, LF_EMPTY_SET, LF_SET(LockFundCacheMixinTraits::MakeId(123), insertRecord.LockFundRecords.cbegin()->first));
		}

		static void AssertMultipleMarkedElementsCanBeTracked() {
			// Arrange:
			LockFundCacheMixinTraits::CacheType cache;
			std::vector<Key> associatedKeys(140);
			auto delta = cache.createDelta(Height{0});
			for (uint8_t i = 100; i < 110; ++i)
			{
				auto record = LockFundCacheMixinTraits::CreateWithId(i);
				associatedKeys[i] = record.LockFundRecords.cbegin()->first;
				delta->insert(record);
			}

			cache.commit();

			// Act:
			// - add two
			delta->insert(LockFundCacheMixinTraits::CreateWithId(123));
			delta->insert(LockFundCacheMixinTraits::CreateWithId(128));

			// - modify three
			delta->find(LockFundCacheMixinTraits::MakeId(105)).get();
			delta->find(LockFundCacheMixinTraits::MakeId(107)).get();
			delta->find(LockFundCacheMixinTraits::MakeId(108)).get();

			// - remove four
			delta->remove(LockFundCacheMixinTraits::MakeId(100));
			delta->remove(LockFundCacheMixinTraits::MakeId(101));
			delta->remove(LockFundCacheMixinTraits::MakeId(104));
			delta->remove(LockFundCacheMixinTraits::MakeId(106));

			// Assert
			AssertMarkedElements(
					*delta,
					LF_SET((LockFundCacheMixinTraits::MakeId(123), LockFundCacheMixinTraits::MakeId(128)),
						   (associatedKeys[123], associatedKeys[128])),
					LF_SET((LockFundCacheMixinTraits::MakeId(105), LockFundCacheMixinTraits::MakeId(107), LockFundCacheMixinTraits::MakeId(108)),
						   (associatedKeys[105], associatedKeys[107], associatedKeys[108])),
					LF_SET((LockFundCacheMixinTraits::MakeId(100), LockFundCacheMixinTraits::MakeId(101), LockFundCacheMixinTraits::MakeId(104), LockFundCacheMixinTraits::MakeId(106)),
						   (associatedKeys[100], associatedKeys[101], associatedKeys[104], associatedKeys[106])));
		}

	}

#undef LF_EMPTY_SET;
#undef LF_SET;
	// region delta elements tests


	// endregion

	// region delta modifier

	TEST(TEST_CLASS, AddingRecordAddsBothIndexes) {
		// Arrange:
		auto record = test::GenerateRecordGroup<state::LockFundHeightIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(Height(0), 1);
		LockFundCacheMixinTraits::CacheType cache;
		{

			auto delta = cache.createDelta(Height{0});
			delta->insert(record.LockFundRecords.begin()->first, record.Identifier,  record.LockFundRecords.begin()->second.Get());

			EXPECT_EQ(delta->size(), 1);
			EXPECT_EQ(delta->secondarySize(), 1);

			cache.commit();
		}

		auto view = cache.createView(Height{0});
		EXPECT_EQ(view->size(), 1);
		EXPECT_EQ(view->secondarySize(), 1);
		EXPECT_TRUE(view->contains(record.LockFundRecords.begin()->first));
		EXPECT_TRUE(view->contains(record.Identifier));
	}

	TEST(TEST_CLASS, AddingRecordWithExistingHeightIndexAppends) {
		// Arrange:
		auto recordGroup = test::GenerateRecordGroup<state::LockFundHeightIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(Height(0), 3);
		LockFundCacheMixinTraits::CacheType cache;
		{

			auto delta = cache.createDelta(Height{0});
			for(auto& record : recordGroup.LockFundRecords)
			{
				delta->insert(record.first, recordGroup.Identifier,  record.second.Get());
			}

			EXPECT_EQ(delta->size(), 1);
			EXPECT_EQ(delta->secondarySize(), 3);

			auto value = delta->find(recordGroup.Identifier).get();
			EXPECT_EQ(value.LockFundRecords.size(), 3);
			cache.commit();
		}

		auto view = cache.createView(Height{0});
		EXPECT_EQ(view->size(), 1);
		EXPECT_EQ(view->secondarySize(), 3);

		for(auto& record : recordGroup.LockFundRecords)
		{
			EXPECT_TRUE(view->contains(record.first));
		}

		EXPECT_TRUE(view->contains(recordGroup.Identifier));
	}

	TEST(TEST_CLASS, AddingRecordWithExistingKeyIndexAppends) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto recordGroup = test::GenerateRecordGroup<state::LockFundKeyIndexDescriptor, test::DefaultRecordGroupGeneratorTraits<state::LockFundKeyIndexDescriptor>>(key, 3);
		LockFundCacheMixinTraits::CacheType cache;
		{

			auto delta = cache.createDelta(Height{0});
			for(auto& record : recordGroup.LockFundRecords)
			{
				delta->insert(recordGroup.Identifier, record.first, record.second.Get());
			}

			EXPECT_EQ(delta->size(), 3);
			EXPECT_EQ(delta->secondarySize(), 1);

			auto value = delta->find(recordGroup.Identifier).get();
			EXPECT_EQ(value.LockFundRecords.size(), 3);
			cache.commit();
		}

		// Assert: root + 2 children, one renewal
		auto view = cache.createView(Height{0});
		EXPECT_EQ(view->size(), 3);
		EXPECT_EQ(view->secondarySize(), 1);

		for(auto& record : recordGroup.LockFundRecords)
		{
			EXPECT_TRUE(view->contains(record.first));
		}

		EXPECT_TRUE(view->contains(recordGroup.Identifier));
	}

	// endregion

	// region DELTA_VIEW_BASED_TEST

	namespace {

		struct KeySupplierRecordGroupGeneratorTraits : public test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>
		{
			static typename state::LockFundHeightIndexDescriptor::ValueIdentifier GenerateIdentifier(uint32_t index)
			{
				auto key = Key();
				*key.begin() =  *key.begin() | index;
				return key;
			}
		};

		template<typename TGeneratorTraits>
		void DefaultPopulateCache(LockedCacheDelta<LockFundCacheDelta>& delta, std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>>& records, uint32_t expectedSecondaryCount = 9)
		{
			records.push_back(test::GenerateRecordGroup<state::LockFundHeightIndexDescriptor, TGeneratorTraits>(Height(10), 3));
			records.push_back(test::GenerateRecordGroup<state::LockFundHeightIndexDescriptor, TGeneratorTraits>(Height(11), 3));
			records.push_back(test::GenerateRecordGroup<state::LockFundHeightIndexDescriptor, TGeneratorTraits>(Height(12), 3));

			delta->insert(records[0]);
			delta->insert(records[1]);
			delta->insert(records[2]);

			EXPECT_EQ(delta->size(), 3);
			EXPECT_EQ(delta->secondarySize(), expectedSecondaryCount);

			for(auto& record : records)
			{
				auto value = delta->find(record.Identifier).get();
				EXPECT_EQ(value.LockFundRecords.size(), record.LockFundRecords.size());
				for(auto lockFundRecord : value.LockFundRecords)
				{
					EXPECT_EQ(lockFundRecord.second.Active(), record.LockFundRecords.find(lockFundRecord.first)->second.Active());
					EXPECT_EQ(lockFundRecord.second.Size(), record.LockFundRecords.find(lockFundRecord.first)->second.Size());
				}
			}
		}

		struct ViewTraits {
			template<typename TAction>
			static void RunTest(TAction action) {
				// Arrange:
				LockFundCacheMixinTraits::CacheType cache;

				std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
				{
					auto delta = cache.createDelta(Height{0});
					DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);
					cache.commit();
				}
				// Act:
				auto view = cache.createView(Height{0});
				action(view, records);
			}
		};

		struct DeltaTraits {
			template<typename TAction>
			static void RunTest(TAction action) {
				// Arrange:
				LockFundCacheMixinTraits::CacheType cache;
				std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
				auto delta = cache.createDelta(Height{0});
				DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);

				// Act:
				action(delta, records);
			}
		};
	}

#define DELTA_VIEW_BASED_TEST(TEST_NAME) \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)(); \
	TEST(TEST_CLASS, TEST_NAME##_View) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<ViewTraits>(); } \
	TEST(TEST_CLASS, TEST_NAME##_Delta) { TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)<DeltaTraits>(); } \
	template<typename TTraits> void TRAITS_TEST_NAME(TEST_CLASS, TEST_NAME)()

	DELTA_VIEW_BASED_TEST(GetReturnsExpectedHeightRecords) {
		// Act:
		TTraits::RunTest([](auto& view, const std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>>& records) {
			auto& entry = view->find(Height(10)).get();
			auto& entry2 = view->find(Height(11)).get();
			auto& entry3 = view->find(Height(12)).get();
			// Assert:
			test::AssertEqual(records[0], entry);
			test::AssertEqual(records[1], entry2);
			test::AssertEqual(records[2], entry3);
		});
	}

	DELTA_VIEW_BASED_TEST(GetReturnsExpectedKeyRecords) {
		// Act:
		TTraits::RunTest([](auto& view, const std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>>& records) {
			for(auto &recordGroup : records)
			{
				for(auto& record : recordGroup.LockFundRecords)
				{
					auto& cacheRecord = view->find(record.first).get();
					test::AssertEqual(cacheRecord.LockFundRecords.begin()->second, record.second);
				}
			}
		});
	}

	DELTA_VIEW_BASED_TEST(TryGetReturnsNullptrWhenHeightIsUnknown) {
		// Act:
		TTraits::RunTest([](auto& view, const std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>>& records) {
			auto namespaceIter = view->find(Height(123));
			const auto* pEntry = namespaceIter.tryGet();

			// Assert:
			EXPECT_FALSE(!!pEntry);
		});
	}

	// endregion

	// region remove

	TEST(TEST_CLASS, CannotRemoveUnknownRecord) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);

		// Act + Assert:
		// Valid height invalid key
		EXPECT_THROW(delta->remove(test::GenerateRandomByteArray<Key>(), Height(10)), catapult_invalid_argument);
		// Invalid both
		EXPECT_THROW(delta->remove(test::GenerateRandomByteArray<Key>(), Height(100)), catapult_invalid_argument);
		// Valid key invalid height
		EXPECT_THROW(delta->remove(records[0].LockFundRecords.begin()->first, Height(100)), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CannotRemoveUnknownHeight) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);

		// Act + Assert:
		EXPECT_THROW(delta->remove(Height(100)), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CannotRecoverUnknownHeight) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);

		// Act + Assert:
		EXPECT_THROW(delta->recover(Height(100)), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CannotDisableUnknownRecord) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);

		// Act + Assert:
		// Valid height invalid key
		EXPECT_THROW(delta->disable(test::GenerateRandomByteArray<Key>(), Height(10)), catapult_invalid_argument);
		// Invalid both
		EXPECT_THROW(delta->disable(test::GenerateRandomByteArray<Key>(), Height(100)), catapult_invalid_argument);
		// Valid key invalid height
		EXPECT_THROW(delta->disable(records[0].LockFundRecords.begin()->first, Height(100)), catapult_invalid_argument);
	}
	TEST(TEST_CLASS, CannotEnableUnknownRecord) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);

		// Act + Assert:
		// Valid height invalid key
		EXPECT_THROW(delta->recover(test::GenerateRandomByteArray<Key>(), Height(10)), catapult_invalid_argument);
		// Invalid both
		EXPECT_THROW(delta->recover(test::GenerateRandomByteArray<Key>(), Height(100)), catapult_invalid_argument);
		// Valid key invalid height
		EXPECT_THROW(delta->recover(records[0].LockFundRecords.begin()->first, Height(100)), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CannotPruneUnknownHeight) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);

		// Act + Assert:
		EXPECT_THROW(delta->remove(Height(100)), catapult_invalid_argument);
	}


	TEST(TEST_CLASS, CanRemoveKnownRecord) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);
		auto removeKey = records[0].LockFundRecords.begin()->first;
		// Act:
		delta->remove(removeKey, Height(10));

		// Assert:
		EXPECT_EQ(delta->size(), 3);
		EXPECT_EQ(delta->secondarySize(), 8);
		EXPECT_FALSE(delta->contains(removeKey));

		auto recordGroup = delta->find(Height(10)).get();

		EXPECT_EQ(recordGroup.LockFundRecords.size(), 2);
		EXPECT_EQ(recordGroup.LockFundRecords.find(removeKey), recordGroup.LockFundRecords.end());
	}

	TEST(TEST_CLASS, RemovingAllHeightGroupRecordsClearsGroup) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);

		auto count = 3;
		for(auto& innerRecord : records[0].LockFundRecords)
		{
			// Act:
			delta->remove(innerRecord.first, Height(10));
			count--;
			// Sanity:

			EXPECT_EQ(delta->secondarySize(), 9 - (3-count));
			EXPECT_FALSE(delta->contains(innerRecord.first));
		}

		EXPECT_EQ(delta->size(), 2);
		auto recordGroup = delta->find(Height(10)).tryGet();
		EXPECT_FALSE(recordGroup);
	}

	TEST(TEST_CLASS, RemovingAllKeyGroupRecordsClearsGroup) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<KeySupplierRecordGroupGeneratorTraits>(delta, records, 3);

		auto firstKey = records[0].LockFundRecords.begin()->first;
		auto keyRecord = delta->find(firstKey).get();

		// Sanity:
		EXPECT_EQ(delta->secondarySize(), 3);
		EXPECT_EQ(keyRecord.LockFundRecords.size(), 3);

		auto count = 3;
		for(auto& innerRecord : records)
		{
			// Act:
			delta->remove(firstKey, innerRecord.Identifier);
			count--;
			// Sanity:
			auto cacheRecord = delta->find(innerRecord.Identifier).get();
			EXPECT_EQ(cacheRecord.LockFundRecords.size(), 2);
		}


		EXPECT_EQ(delta->size(), 3);
		EXPECT_EQ(delta->secondarySize(), 2);
		auto recordGroup = delta->find(firstKey).tryGet();
		EXPECT_FALSE(recordGroup);
	}

	TEST(TEST_CLASS, CanDisableKnownRecord) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);
		auto removeKey = records[0].LockFundRecords.begin()->first;
		// Act:
		delta->disable(removeKey, Height(10));

		// Assert:
		EXPECT_EQ(delta->size(), 3);
		EXPECT_EQ(delta->secondarySize(), 9);
		EXPECT_TRUE(delta->contains(removeKey));

		auto firstKeyRecord = delta->find(removeKey).get().LockFundRecords.begin()->second;

		EXPECT_FALSE(firstKeyRecord.Active());
		EXPECT_EQ(firstKeyRecord.InactiveRecords.size(), 3);

		auto firstHeightRecord = delta->find(Height(10)).get().LockFundRecords.find(removeKey)->second;

		EXPECT_FALSE(firstHeightRecord.Active());
		EXPECT_EQ(firstHeightRecord.InactiveRecords.size(), 3);
	}

	TEST(TEST_CLASS, CanEnableKnownRecord) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);
		auto removeKey = records[0].LockFundRecords.begin()->first;
		// Act:
		delta->recover(removeKey, Height(10));

		// Assert:
		EXPECT_EQ(delta->size(), 3);
		EXPECT_EQ(delta->secondarySize(), 9);
		EXPECT_TRUE(delta->contains(removeKey));

		auto firstRecord = delta->find(removeKey).get().LockFundRecords.begin()->second;

		EXPECT_TRUE(firstRecord.Active());
		EXPECT_EQ(firstRecord.InactiveRecords.size(), 1);
	}

	TEST(TEST_CLASS, CanPruneHeight) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);
		// Act:
		delta->remove(Height(10));

		// Assert:
		EXPECT_EQ(delta->size(), 2);
		EXPECT_EQ(delta->secondarySize(), 6);
		EXPECT_FALSE(delta->contains(Height(10)));


		for(auto& innerRecord : records[0].LockFundRecords)
		{
			auto recordKeyGroup = delta->find(innerRecord.first).tryGet();
			EXPECT_FALSE(recordKeyGroup);
			EXPECT_FALSE(delta->contains(innerRecord.first));
		}
	}
	TEST(TEST_CLASS, CanDisableHeight) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);
		// Act:
		delta->disable(Height(10));

		// Assert:
		EXPECT_EQ(delta->size(), 3);
		EXPECT_EQ(delta->secondarySize(), 9);
		EXPECT_TRUE(delta->contains(Height(10)));

		auto recordHeightGroup = delta->find(Height(10)).get();

		EXPECT_EQ(recordHeightGroup.LockFundRecords.size(), 3);
		for(auto innerCacheRecord : recordHeightGroup.LockFundRecords)
		{
			EXPECT_EQ(innerCacheRecord.second.Size(), 3);
			EXPECT_FALSE(innerCacheRecord.second.Active());
		}
		for(auto& innerRecord : records[0].LockFundRecords)
		{
			auto recordKeyGroup = delta->find(innerRecord.first).get();
			EXPECT_EQ(recordKeyGroup.LockFundRecords.begin()->second.Size(), 3);
			EXPECT_FALSE(recordKeyGroup.LockFundRecords.begin()->second.Active());
		}
	}

	TEST(TEST_CLASS, CanRecoverHeight) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);
		// Act:
		delta->recover(Height(10));

		// Assert:
		EXPECT_EQ(delta->size(), 3);
		EXPECT_EQ(delta->secondarySize(), 9);
		EXPECT_TRUE(delta->contains(Height(10)));

		auto recordHeightGroup = delta->find(Height(10)).get();

		EXPECT_EQ(recordHeightGroup.LockFundRecords.size(), 3);
		for(auto innerCacheRecord : recordHeightGroup.LockFundRecords)
		{
			EXPECT_EQ(innerCacheRecord.second.Size(), 1);
			EXPECT_TRUE(innerCacheRecord.second.Active());
		}
		for(auto& innerRecord : records[0].LockFundRecords)
		{
			auto recordKeyGroup = delta->find(innerRecord.first).get();
			EXPECT_EQ(recordKeyGroup.LockFundRecords.begin()->second.Size(), 1);
			EXPECT_TRUE(recordKeyGroup.LockFundRecords.begin()->second.Active());
		}
	}

	TEST(TEST_CLASS, CanActAndToggleWithRemoveHeight) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);
		// Act:
		delta->actAndToggle(Height(10), false, [](const auto&, const auto&){});

		// Assert:
		EXPECT_EQ(delta->size(), 3);
		EXPECT_EQ(delta->secondarySize(), 9);
		EXPECT_TRUE(delta->contains(Height(10)));

		auto recordHeightGroup = delta->find(Height(10)).get();

		EXPECT_EQ(recordHeightGroup.LockFundRecords.size(), 3);
		for(auto innerCacheRecord : recordHeightGroup.LockFundRecords)
		{
			EXPECT_EQ(innerCacheRecord.second.Size(), 3);
			EXPECT_FALSE(innerCacheRecord.second.Active());
		}
		for(auto& innerRecord : records[0].LockFundRecords)
		{
			auto recordKeyGroup = delta->find(innerRecord.first).get();
			EXPECT_EQ(recordKeyGroup.LockFundRecords.begin()->second.Size(), 3);
			EXPECT_FALSE(recordKeyGroup.LockFundRecords.begin()->second.Active());
		}
	}

	TEST(TEST_CLASS, CanActAndToggleWithRecoverHeight) {
		// Arrange:
		LockFundCacheMixinTraits::CacheType cache;
		std::vector<state::LockFundRecordGroup<state::LockFundHeightIndexDescriptor>> records;
		auto delta = cache.createDelta(Height{0});
		DefaultPopulateCache<test::DefaultRecordGroupGeneratorTraits<state::LockFundHeightIndexDescriptor>>(delta, records);
		// Act:
		delta->actAndToggle(Height(10), true, [](const auto&, const auto&){});

		// Assert:
		EXPECT_EQ(delta->size(), 3);
		EXPECT_EQ(delta->secondarySize(), 9);
		EXPECT_TRUE(delta->contains(Height(10)));

		auto recordHeightGroup = delta->find(Height(10)).get();

		EXPECT_EQ(recordHeightGroup.LockFundRecords.size(), 3);
		for(auto innerCacheRecord : recordHeightGroup.LockFundRecords)
		{
			EXPECT_EQ(innerCacheRecord.second.Size(), 1);
			EXPECT_TRUE(innerCacheRecord.second.Active());
		}
		for(auto& innerRecord : records[0].LockFundRecords)
		{
			auto recordKeyGroup = delta->find(innerRecord.first).get();
			EXPECT_EQ(recordKeyGroup.LockFundRecords.begin()->second.Size(), 1);
			EXPECT_TRUE(recordKeyGroup.LockFundRecords.begin()->second.Active());
		}
	}

	// endregion


}}
