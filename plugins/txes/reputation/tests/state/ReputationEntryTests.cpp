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

#include "src/state/ReputationEntry.h"
#include "tests/test/ReputationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS ReputationEntryTests

	namespace {
		void AssertEntry(uint64_t expectedPositiveInteractions,
				uint64_t expectedNegativeInteractions, const ReputationEntry& entry) {
			EXPECT_EQ(expectedPositiveInteractions, entry.positiveInteractions().unwrap());
			EXPECT_EQ(expectedNegativeInteractions, entry.negativeInteractions().unwrap());
		}
	}

	TEST(TEST_CLASS, CanCreateReputationEntry) {
		// Act:
		auto key = test::GenerateRandomData<Key_Size>();
		auto entry = ReputationEntry(key);

		// Assert:
		EXPECT_EQ(key, entry.key());
		AssertEntry(0, 0, entry);
	}

	TEST(TEST_CLASS, CanSetPositiveInteractions) {
		// Arrange:
		auto key = test::GenerateRandomData<Key_Size>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setPositiveInteractions(Reputation{10});

		// Assert:
		AssertEntry(10, 0, entry);
	}

	TEST(TEST_CLASS, CanIncrementPositiveInteractions) {
		// Arrange:
		auto key = test::GenerateRandomData<Key_Size>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setPositiveInteractions(Reputation{10});
		entry.incrementPositiveInteractions();

		// Assert:
		AssertEntry(11, 0, entry);
	}

	TEST(TEST_CLASS, CanDecrementPositiveInteractions) {
		// Arrange:
		auto key = test::GenerateRandomData<Key_Size>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setPositiveInteractions(Reputation{10});
		entry.decrementPositiveInteractions();

		// Assert:
		AssertEntry(9, 0, entry);
	}

	TEST(TEST_CLASS, CanSetNegativeInteractions) {
		// Arrange:
		auto key = test::GenerateRandomData<Key_Size>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setNegativeInteractions(Reputation{20});

		// Assert:
		AssertEntry(0, 20, entry);
	}

	TEST(TEST_CLASS, CanIncrementNegativeInteractions) {
		// Arrange:
		auto key = test::GenerateRandomData<Key_Size>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setNegativeInteractions(Reputation{20});
		entry.incrementNegativeInteractions();

		// Assert:
		AssertEntry(0, 21, entry);
	}

	TEST(TEST_CLASS, CanDecrementNegativeInteractions) {
		// Arrange:
		auto key = test::GenerateRandomData<Key_Size>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setNegativeInteractions(Reputation{20});
		entry.decrementNegativeInteractions();

		// Assert:
		AssertEntry(0, 19, entry);
	}
}}
