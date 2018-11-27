/**
*** Copyright (c) 2016-present,
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

#include "catapult/cache/IdentifierGroupCacheUtils.h"
#include "catapult/cache/CacheDescriptorAdapters.h"
#include "catapult/utils/Hashers.h"
#include "tests/catapult/cache/test/UnsupportedSerializer.h"
#include "tests/TestHarness.h"

namespace catapult { namespace cache {

#define TEST_CLASS IdentifierGroupCacheUtilsTests

	namespace {
		// int grouped by Height
		class TestIdentifierGroup : public utils::IdentifierGroup<int, Height, std::hash<int>> {
		public:
#ifdef _MSC_VER
			TestIdentifierGroup() : TestIdentifierGroup(Height())
			{}
#endif

			using utils::IdentifierGroup<int, Height, std::hash<int>>::IdentifierGroup;
		};

		TestIdentifierGroup AddValues(TestIdentifierGroup&& group, std::initializer_list<int> values) {
			for (auto value : values)
				group.add(value);

			return group;
		}

		struct TestHeightGroupedCacheDescriptor {
			using KeyType = Height;
			using ValueType = TestIdentifierGroup;

			using Serializer = test::UnsupportedSerializer<KeyType, ValueType>;

			static KeyType GetKeyFromValue(const ValueType& value) {
				return value.key();
			}
		};

		template<typename TBaseSet>
		class BaseSetTypeWrapper : public TBaseSet {
		public:
			BaseSetTypeWrapper() : TBaseSet(deltaset::ConditionalContainerMode::Memory, m_database, 0)
			{}

		private:
			CacheDatabase m_database;
		};

		using HeightGroupedTypes = MutableUnorderedMapAdapter<TestHeightGroupedCacheDescriptor, utils::BaseValueHasher<Height>>;
		using HeightGroupedBaseSetType = BaseSetTypeWrapper<HeightGroupedTypes::BaseSetType>;

		// fake cache that indexes strings by size
		struct TestCacheDescriptor {
			using KeyType = int;
			using ValueType = std::string;
			using Serializer = test::UnsupportedSerializer<KeyType, ValueType>;

			static KeyType GetKeyFromValue(const ValueType& value) {
				return static_cast<int>(value.size());
			}
		};

		using BasicTypes = MutableUnorderedMapAdapter<TestCacheDescriptor>;
		using BaseSetType = BaseSetTypeWrapper<BasicTypes::BaseSetType>;
	}

	// region AddIdentifierWithGroup

	TEST(TEST_CLASS, AddIdentifierWithGroup_AddsIdentifierToNewGroup) {
		// Arrange:
		HeightGroupedBaseSetType groupedSet;
		auto pGroupedDelta = groupedSet.rebase();
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(3)), { 1, 4, 9 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(7)), { 25, 26 }));

		// Sanity:
		EXPECT_FALSE(pGroupedDelta->contains(Height(5)));

		// Act:
		AddIdentifierWithGroup(*pGroupedDelta, Height(5), 17);

		// Assert:
		const auto* pGroup = pGroupedDelta->find(Height(5)).get();
		ASSERT_TRUE(!!pGroup);
		EXPECT_EQ(TestIdentifierGroup::Identifiers({ 17 }), pGroup->identifiers());
	}

	TEST(TEST_CLASS, AddIdentifierWithGroup_AddsIdentifierToExistingGroup) {
		// Arrange:
		HeightGroupedBaseSetType groupedSet;
		auto pGroupedDelta = groupedSet.rebase();
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(3)), { 1, 4, 9 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(7)), { 25, 26 }));

		// Act:
		AddIdentifierWithGroup(*pGroupedDelta, Height(3), 7);

		// Assert:
		const auto* pGroup = pGroupedDelta->find(Height(3)).get();
		ASSERT_TRUE(!!pGroup);
		EXPECT_EQ(TestIdentifierGroup::Identifiers({ 1, 4, 7, 9 }), pGroup->identifiers());
	}

	TEST(TEST_CLASS, AddIdentifierWithGroup_HasNoEffectWhenAddingExistingIdentifierToExistingGroup) {
		// Arrange:
		HeightGroupedBaseSetType groupedSet;
		auto pGroupedDelta = groupedSet.rebase();
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(3)), { 1, 4, 9 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(7)), { 25, 26 }));

		// Act:
		AddIdentifierWithGroup(*pGroupedDelta, Height(3), 4);

		// Assert:
		const auto* pGroup = pGroupedDelta->find(Height(3)).get();
		ASSERT_TRUE(!!pGroup);
		EXPECT_EQ(TestIdentifierGroup::Identifiers({ 1, 4, 9 }), pGroup->identifiers());
	}

	// endregion

	// region ForEachIdentifierWithGroup

	TEST(TEST_CLASS, ForEachIdentifierWithGroup_DoesNotCallActionWhenNoIdentifiersInGroup) {
		// Arrange:
		BaseSetType set;
		auto pDelta = set.rebase();
		pDelta->insert("a");
		pDelta->insert(std::string(25, 'z'));

		HeightGroupedBaseSetType groupedSet;
		auto pGroupedDelta = groupedSet.rebase();
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(1)), { 100 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(3)), { 1, 4, 9 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(7)), { 25, 26 }));

		// Sanity:
		EXPECT_FALSE(pGroupedDelta->contains(Height(5)));

		// Act:
		auto numActionCalls = 0u;
		ForEachIdentifierWithGroup(*pDelta, *pGroupedDelta, Height(5), [&numActionCalls](const auto&) {
			++numActionCalls;
		});

		// Assert:
		EXPECT_EQ(0u, numActionCalls);
	}

	TEST(TEST_CLASS, ForEachIdentifierWithGroup_CallsActionForAllValuesInGroup) {
		// Arrange:
		BaseSetType set;
		auto pDelta = set.rebase();
		pDelta->insert("a");
		pDelta->insert("bbbb");
		pDelta->insert(std::string(9, 'c'));
		pDelta->insert(std::string(100, 'z'));

		HeightGroupedBaseSetType groupedSet;
		auto pGroupedDelta = groupedSet.rebase();
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(1)), { 100 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(3)), { 1, 4, 9 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(7)), { 25, 26 }));

		// Act:
		auto numActionCalls = 0u;
		std::unordered_set<std::string> values;
		ForEachIdentifierWithGroup(*pDelta, *pGroupedDelta, Height(3), [&numActionCalls, &values](const auto& str) {
			++numActionCalls;
			values.insert(str);
		});

		// Assert:
		EXPECT_EQ(3u, numActionCalls);

		EXPECT_EQ(3u, values.size());
		EXPECT_TRUE(values.cend() != values.find("a"));
		EXPECT_TRUE(values.cend() != values.find("bbbb"));
		EXPECT_TRUE(values.cend() != values.find(std::string(9, 'c')));
	}

	TEST(TEST_CLASS, ForEachIdentifierWithGroup_CallsActionForAllValuesInGroupAndIgnoresUnknownValues) {
		// Arrange:
		BaseSetType set;
		auto pDelta = set.rebase();
		pDelta->insert("a");
		pDelta->insert(std::string(9, 'c'));
		pDelta->insert(std::string(100, 'z'));

		HeightGroupedBaseSetType groupedSet;
		auto pGroupedDelta = groupedSet.rebase();
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(1)), { 100 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(3)), { 1, 4, 9 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(7)), { 25, 26 }));

		// Act:
		auto numActionCalls = 0u;
		std::unordered_set<std::string> values;
		ForEachIdentifierWithGroup(*pDelta, *pGroupedDelta, Height(3), [&numActionCalls, &values](const auto& str) {
			++numActionCalls;
			values.insert(str);
		});

		// Assert: value with id 4 is in group but not in underlying set
		EXPECT_EQ(2u, numActionCalls);

		EXPECT_EQ(2u, values.size());
		EXPECT_TRUE(values.cend() != values.find("a"));
		EXPECT_TRUE(values.cend() != values.find(std::string(9, 'c')));
	}

	// endregion

	// region RemoveAllIdentifiersWithGroup

	TEST(TEST_CLASS, RemoveAllIdentifiersWithGroup_DoesNotRemoveAnythingWhenNoIdentifiersInGroup) {
		// Arrange:
		BaseSetType set;
		auto pDelta = set.rebase();
		pDelta->insert("a");
		pDelta->insert(std::string(25, 'z'));

		HeightGroupedBaseSetType groupedSet;
		auto pGroupedDelta = groupedSet.rebase();
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(1)), { 100 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(3)), { 1, 4, 9 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(7)), { 25, 26 }));

		// Sanity:
		EXPECT_FALSE(pGroupedDelta->contains(Height(5)));

		// Act:
		RemoveAllIdentifiersWithGroup(*pDelta, *pGroupedDelta, Height(5));

		// Assert: nothing was removed
		EXPECT_EQ(2u, pDelta->size());
		EXPECT_EQ(3u, pGroupedDelta->size());
	}

	TEST(TEST_CLASS, RemoveAllIdentifiersWithGroup_RemovesAllValuesInGroup) {
		// Arrange:
		BaseSetType set;
		auto pDelta = set.rebase();
		pDelta->insert("a");
		pDelta->insert("bbbb");
		pDelta->insert(std::string(9, 'c'));
		pDelta->insert(std::string(100, 'z'));

		HeightGroupedBaseSetType groupedSet;
		auto pGroupedDelta = groupedSet.rebase();
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(1)), { 100 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(3)), { 1, 4, 9 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(7)), { 25, 26 }));

		// Act:
		RemoveAllIdentifiersWithGroup(*pDelta, *pGroupedDelta, Height(3));

		// Assert:
		EXPECT_EQ(1u, pDelta->size());
		EXPECT_TRUE(pDelta->contains(100));

		EXPECT_EQ(2u, pGroupedDelta->size());
		EXPECT_TRUE(pGroupedDelta->contains(Height(1)));
		EXPECT_TRUE(pGroupedDelta->contains(Height(7)));
	}

	TEST(TEST_CLASS, RemoveAllIdentifiersWithGroup_RemovesAllValuesInGroupAndIgnoresUnknownValues) {
		// Arrange:
		BaseSetType set;
		auto pDelta = set.rebase();
		pDelta->insert("a");
		pDelta->insert(std::string(9, 'c'));
		pDelta->insert(std::string(100, 'z'));

		HeightGroupedBaseSetType groupedSet;
		auto pGroupedDelta = groupedSet.rebase();
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(1)), { 100 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(3)), { 1, 4, 9 }));
		pGroupedDelta->insert(AddValues(TestIdentifierGroup(Height(7)), { 25, 26 }));

		// Act:
		RemoveAllIdentifiersWithGroup(*pDelta, *pGroupedDelta, Height(3));

		// Assert:
		EXPECT_EQ(1u, pDelta->size());
		EXPECT_TRUE(pDelta->contains(100));

		EXPECT_EQ(2u, pGroupedDelta->size());
		EXPECT_TRUE(pGroupedDelta->contains(Height(1)));
		EXPECT_TRUE(pGroupedDelta->contains(Height(7)));
	}

	// endregion

}}
