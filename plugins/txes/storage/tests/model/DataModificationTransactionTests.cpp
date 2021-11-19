/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/DataModificationTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = DataModificationTransaction;

#define TEST_CLASS DataModificationTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ Key_Size // drive key size
					+ Hash256_Size // CDI modification size
					+ sizeof(uint64_t) // upload size
					+ sizeof(Amount); // feedback fee

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 80u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_DataModification, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(DataModification)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		DataModificationTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = DataModificationTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(DataModificationTransaction), realSize);
	}

	// endregion
}}
