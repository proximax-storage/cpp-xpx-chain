/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/ReplicatorOffboardingTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = ReplicatorOffboardingTransaction;

#define TEST_CLASS ReplicatorOffboardingTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize; // base

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_ReplicatorOffboarding, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(ReplicatorOffboarding)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		ReplicatorOffboardingTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = ReplicatorOffboardingTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(ReplicatorOffboardingTransaction), realSize);
	}

	// endregion
}}
