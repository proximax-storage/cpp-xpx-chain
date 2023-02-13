/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/DeployContractTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

    using TransactionType = DeployContractTransaction;

#define TEST_CLASS DeployContractTransactionTests

    // region size + properties

    namespace {
        template<typename T>
        void AssertEntityHasExpectedSize(size_t baseSize) {
            // Arrange:
            auto expectedSize = 
                    baseSize // base
                    + Key_Size // drive key
                    + sizeof(uint16_t) // fileName size
                    + sizeof(uint16_t) // functionName size
                    + sizeof(uint16_t) // actualArguments size
                    + sizeof(Amount) // executionCall payment
                    + sizeof(Amount) // downloadCall payment
                    + 1 // servicePayments count
                    + sizeof(uint16_t) // automaticExecutionFileName size
                    + sizeof(uint16_t) // automaticExecutionFunctionName size
                    + sizeof(Amount) // automaticExecutionCall payment
                    + sizeof(Amount) // automaticDownloadCall payment
                    + sizeof(uint32_t) // automaticExecutions number
                    + Key_Size; // assignee
            
            // Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 111u, sizeof(T));
        }

        template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_DeployContractTransaction, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
    }

    ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(DeployContract)

    // endregion

    // region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		DeployContractTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = DeployContractTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(DeployContractTransaction), realSize);
	}

	// endregion
}}