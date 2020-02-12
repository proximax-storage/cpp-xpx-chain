/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	BasicOperationTestTraits::ValueType BasicOperationTestTraits::CreateLockInfo(Height height, state::LockStatus status) {
		return CreateOperationEntry(GenerateRandomByteArray<Hash256>(), height, status);
	}

	BasicOperationTestTraits::ValueType BasicOperationTestTraits::CreateLockInfo() {
		return CreateLockInfo(test::GenerateRandomValue<Height>());
	}

	void BasicOperationTestTraits::SetKey(ValueType& lockInfo, const KeyType& key) {
		lockInfo.OperationToken = key;
	}

	void BasicOperationTestTraits::AssertEqual(const ValueType& lhs, const ValueType& rhs) {
		AssertEqualOperationData(lhs, rhs);
	}

	state::OperationEntry CreateOperationEntry(
			Hash256 operationToken,
			Height height,
			state::LockStatus status,
			Key initiator,
			uint16_t mosaicCount,
			uint16_t executorCount,
			uint16_t transactionHashCount) {
		state::OperationEntry entry(operationToken);
		entry.Account = initiator;
		entry.Height = height;
		entry.Status = status;
		while (mosaicCount--)
			entry.Mosaics.emplace(test::GenerateRandomValue<MosaicId>(), test::GenerateRandomValue<Amount>());
		while (executorCount--)
			entry.Executors.insert(test::GenerateRandomByteArray<Key>());
		entry.TransactionHashes.reserve(transactionHashCount);
		while (transactionHashCount--)
			entry.TransactionHashes.push_back(test::GenerateRandomByteArray<Hash256>());

		return entry;
	}

	void AssertEqualOperationData(const state::OperationEntry& entry1, const state::OperationEntry& entry2) {
		EXPECT_EQ(entry1.OperationToken, entry2.OperationToken);
		EXPECT_EQ(entry1.Account, entry2.Account);
		EXPECT_EQ(entry1.Height, entry2.Height);
		EXPECT_EQ(entry1.Status, entry2.Status);
		EXPECT_EQ(entry1.Result, entry2.Result);
		ASSERT_EQ(entry1.Mosaics.size(), entry2.Mosaics.size());
		for (const auto& pair : entry1.Mosaics)
			EXPECT_EQ(pair.second, entry2.Mosaics.at(pair.first));
		EXPECT_EQ(entry1.Executors.size(), entry2.Executors.size());
		for (const auto& executor : entry1.Executors)
			EXPECT_EQ(1, entry2.Executors.count(executor));
		auto hashCount = entry1.TransactionHashes.size();
		ASSERT_EQ(hashCount, entry2.TransactionHashes.size());
		EXPECT_EQ_MEMORY(entry1.TransactionHashes.data(), entry2.TransactionHashes.data(), hashCount * Hash256_Size);

	}
}}


