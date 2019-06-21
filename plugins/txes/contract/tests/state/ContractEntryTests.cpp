/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/ContractEntry.h"
#include "tests/test/ReputationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS ContractEntryTests

	TEST(TEST_CLASS, CanCreateContractEntry) {
		// Act:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ContractEntry(key);

		// Assert:
		EXPECT_EQ(key, entry.key());
	}

	TEST(TEST_CLASS, CanSetStart) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ContractEntry(key);

		// Act:
		entry.setStart(Height(12));

		// Assert:
		EXPECT_EQ(12, entry.start().unwrap());
	}

	TEST(TEST_CLASS, CanSetDuration) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ContractEntry(key);

		// Act:
		entry.setDuration(BlockDuration(20));

		// Assert:
		EXPECT_EQ(20, entry.duration().unwrap());
	}

	TEST(TEST_CLASS, CanSetHashs) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ContractEntry(key);

		// Act:
		auto hash1 = test::GenerateRandomByteArray<Hash256>();
		auto hash2 = test::GenerateRandomByteArray<Hash256>();
		entry.pushHash(hash1, Height(1));
		entry.pushHash(hash2, Height(2));

		// Assert:
		EXPECT_EQ(hash2, entry.hash());
		EXPECT_EQ(2, entry.hashes().size());

		entry.popHash();
		EXPECT_EQ(hash1, entry.hash());
		EXPECT_EQ(1, entry.hashes().size());
	}

	TEST(TEST_CLASS, CanSetCustomers) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ContractEntry(key);

		// Act:
		utils::SortedKeySet customers = { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() };
		entry.customers() = customers;

		// Assert:
		EXPECT_EQ(customers, entry.customers());
	}

	TEST(TEST_CLASS, CanSetExecutors) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ContractEntry(key);

		// Act:
		utils::SortedKeySet executors = { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() };
		entry.executors() = executors;

		// Assert:
		EXPECT_EQ(executors, entry.executors());
	}

	TEST(TEST_CLASS, CanSetVerifiers) {
		// Arrange:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = ContractEntry(key);

		// Act:
		utils::SortedKeySet verifiers = { test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() };
		entry.verifiers() = verifiers;

		// Assert:
		EXPECT_EQ(verifiers, entry.verifiers());
	}
}}
