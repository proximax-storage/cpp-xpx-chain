/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/PrepareBcDriveTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = PrepareBcDriveTransaction;

#define TEST_CLASS PrepareBcDriveTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ sizeof(uint64_t) // drive size
					+ sizeof(Amount) // verification fee amount
					+ sizeof(uint16_t); // replicator count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 18u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_PrepareBcDrive, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(PrepareBcDrive)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		PrepareBcDriveTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = PrepareBcDriveTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(PrepareBcDriveTransaction), realSize);
	}

	// endregion
}}
