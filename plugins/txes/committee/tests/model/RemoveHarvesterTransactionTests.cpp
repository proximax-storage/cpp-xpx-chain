/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/RemoveHarvesterTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = RemoveHarvesterTransaction;

#define TEST_CLASS RemoveHarvesterTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			EXPECT_EQ(baseSize, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_RemoveHarvester, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(RemoveHarvester)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TransactionType), realSize);
	}

	// endregion
}}
