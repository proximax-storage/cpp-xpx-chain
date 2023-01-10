/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/EndBatchExecutionSingleTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

    using TransactionType = EndBatchExecutionSingleTransaction;

#define TEST_CLASS EndBatchExecutionSingleTransactionTests

    // region size + properties

    namespace {
        template<typename T>
        void AssertEntityHasExpectedSize(size_t baseSize) {
            // Arrange:
            auto expectedRawProofOfExecutionSize =
                    + sizeof(uint64_t) // start batch id
                    + 32 + 32 + 32 + 32; // T, R, F, K respectively 
            auto expectedSize = 
                    baseSize // base
                    + Key_Size // contract key
                    + sizeof(uint64_t) // batch id
                    + expectedRawProofOfExecutionSize;
            
            // Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 176u, sizeof(T));
        }

        template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_EndBatchExecutionSingleTransaction, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
    }

    ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(EndBatchExecutionSingle)

    // endregion

    // region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		EndBatchExecutionSingleTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = EndBatchExecutionSingleTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(EndBatchExecutionSingleTransaction), realSize);
	}

	// endregion
}}