/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/SuccessfulEndBatchExecutionTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

    using TransactionType = SuccessfulEndBatchExecutionTransaction;

#define TEST_CLASS SuccessfulEndBatchExecutionTransactionTests

    // region size + properties

    namespace {
        template<typename T>
        void AssertEntityHasExpectedSize(size_t baseSize) {
            // Arrange:
            auto expectedSize = 
                    baseSize // base
                    + Key_Size // contract key
                    + sizeof(uint64_t) // batch id
                    + Hash256_Size // storage hash
                    + sizeof(uint64_t) // usedSize bytes
                    + sizeof(uint64_t) // metaFilesSize bytes
                    + 32 // proofOfExecutionVerification information
                    + sizeof(uint64_t) // check next block for automatic executions
                    + sizeof(uint16_t) // number of cosigners
                    + sizeof(uint16_t); // number of calls
            
            // Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 132u, sizeof(T));
        }

        template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_SuccessfulEndBatchExecutionTransaction, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
    }

    ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(SuccessfulEndBatchExecution)

    // endregion

    // region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		SuccessfulEndBatchExecutionTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = SuccessfulEndBatchExecutionTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(SuccessfulEndBatchExecutionTransaction), realSize);
	}

	// endregion
}}