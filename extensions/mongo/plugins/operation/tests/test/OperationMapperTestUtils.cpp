/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "OperationMapperTestUtils.h"
#include "plugins/txes/operation/src/state/OperationEntry.h"
#include "mongo/plugins/lock_shared/tests/test/LockMapperTestUtils.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace test {

	void AssertEqualMongoOperationData(
			const state::OperationEntry& entry,
			const Address& address,
			const bsoncxx::document::view& dbOperationEntry) {
		AssertEqualBaseLockInfoData(entry, address, dbOperationEntry, true);
		EXPECT_EQ(entry.OperationToken, GetHashValue(dbOperationEntry, "token"));
		EXPECT_EQ(entry.Result, GetUint16(dbOperationEntry, "result"));

		const auto& executorArray = dbOperationEntry["executors"].get_array().value;
		ASSERT_EQ(entry.Executors.size(), test::GetFieldCount(executorArray));
		for (const auto& dbExecutor : executorArray) {
			Key executor;
			DbBinaryToModelArray(executor, dbExecutor.get_binary());
			EXPECT_EQ(1, entry.Executors.count(executor));
		}

		const auto& transactionHashArray = dbOperationEntry["transactionHashes"].get_array().value;
		ASSERT_EQ(entry.TransactionHashes.size(), test::GetFieldCount(transactionHashArray));
		auto i = 0u;
		for (const auto& dbTransactionHash : transactionHashArray) {
			Hash256 transactionHash;
			DbBinaryToModelArray(transactionHash, dbTransactionHash.get_binary());
			EXPECT_EQ(entry.TransactionHashes[i++], transactionHash);
		}
	}
}}
