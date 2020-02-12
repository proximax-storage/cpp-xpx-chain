/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <plugins/txes/operation/src/model/OperationTypes.h>
#include "src/state/OperationEntry.h"
#include "plugins/txes/lock_shared/tests/state/LockInfoTests.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS OperationEntryTests

	DEFINE_LOCK_INFO_TESTS(OperationEntry)

	TEST(TEST_CLASS, OperationEntryConstructorSetsOperationToken) {
		// Arrange:
		auto operationToken = test::GenerateRandomByteArray<Hash256>();

		// Act:
		OperationEntry entry(operationToken);

		// Assert:
		EXPECT_EQ(operationToken, entry.OperationToken);
		EXPECT_EQ(Key(), entry.Account);
		EXPECT_EQ(0, entry.Mosaics.size());
		EXPECT_EQ(Height(0), entry.Height);
		EXPECT_EQ(LockStatus::Unused, entry.Status);
		EXPECT_EQ(0, entry.Executors.size());
		EXPECT_EQ(0, entry.TransactionHashes.size());
		EXPECT_EQ(model::Operation_Result_None, entry.Result);
	}
}}
