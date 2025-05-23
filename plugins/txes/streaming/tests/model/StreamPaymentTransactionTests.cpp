/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/model/StreamPaymentTransaction.h>
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = StreamPaymentTransaction;

#define TEST_CLASS StreamPaymentTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ Key_Size // drive key size
				    + Hash256_Size // stream id size
					+ sizeof(uint64_t); // additional upload size

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 72, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_StreamPayment, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(StreamPayment)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		StreamPaymentTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = StreamPaymentTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(StreamPaymentTransaction), realSize);
	}

	// endregion
}}
