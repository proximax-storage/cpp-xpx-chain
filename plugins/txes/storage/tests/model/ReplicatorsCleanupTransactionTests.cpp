/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/ReplicatorsCleanupTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = ReplicatorsCleanupTransaction;

#define TEST_CLASS ReplicatorsCleanupTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
				baseSize // base
				+ sizeof(uint16_t); // opinion element count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 2u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_ReplicatorsCleanup, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(ReplicatorsCleanup)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		ReplicatorsCleanupTransaction transaction;
		transaction.ReplicatorCount = 7;
		transaction.Size = 0;

		// Act:
		auto realSize = ReplicatorsCleanupTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(ReplicatorsCleanupTransaction) + 7 * Key_Size, realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		ReplicatorsCleanupTransaction transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.ReplicatorCount);

		// Act:
		auto realSize = ReplicatorsCleanupTransaction::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(ReplicatorsCleanupTransaction) + 0xFFFF * Key_Size, realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
