/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/AutomaticExecutionsPaymentTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

    using TransactionType = AutomaticExecutionsPaymentTransaction;

#define TEST_CLASS AutomaticExecutionsPaymentTransactionTests

    // region size + properties

    namespace {
        template<typename T>
        void AssertEntityHasExpectedSize(size_t baseSize) {
            // Arrange:
            auto expectedSize = 
                    baseSize // base
                    + Key_Size // drive key
                    + sizeof(uint32_t); // automaticExecutions number
            
            // Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 36u, sizeof(T));
        }

        template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_AutomaticExecutionsPaymentTransaction, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
    }

    ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(AutomaticExecutionsPayment)

    // endregion

    // region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		AutomaticExecutionsPaymentTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = AutomaticExecutionsPaymentTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(AutomaticExecutionsPaymentTransaction), realSize);
	}

	// endregion
}}