/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ReputationEntry(key);

		// Assert:
		EXPECT_EQ(key, entry.key());
		AssertEntry(0, 0, entry);
	}

	TEST(TEST_CLASS, CanSetPositiveInteractions) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setPositiveInteractions(Reputation{10});

		// Assert:
		AssertEntry(10, 0, entry);
	}

	TEST(TEST_CLASS, CanIncrementPositiveInteractions) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setPositiveInteractions(Reputation{10});
		entry.incrementPositiveInteractions();

		// Assert:
		AssertEntry(11, 0, entry);
	}

	TEST(TEST_CLASS, CanDecrementPositiveInteractions) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setPositiveInteractions(Reputation{10});
		entry.decrementPositiveInteractions();

		// Assert:
		AssertEntry(9, 0, entry);
	}

	TEST(TEST_CLASS, CanSetNegativeInteractions) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setNegativeInteractions(Reputation{20});

		// Assert:
		AssertEntry(0, 20, entry);
	}

	TEST(TEST_CLASS, CanIncrementNegativeInteractions) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setNegativeInteractions(Reputation{20});
		entry.incrementNegativeInteractions();

		// Assert:
		AssertEntry(0, 21, entry);
	}

	TEST(TEST_CLASS, CanDecrementNegativeInteractions) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ReputationEntry(key);

		// Act:
		entry.setNegativeInteractions(Reputation{20});
		entry.decrementNegativeInteractions();

		// Assert:
		AssertEntry(0, 19, entry);
	}
}}
