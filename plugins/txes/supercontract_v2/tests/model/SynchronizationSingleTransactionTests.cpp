/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/SynchronizationSingleTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

    using TransactionType = SynchronizationSingleTransaction;

#define TEST_CLASS SynchronizationSingleTransactionTests

    // region size + properties

    namespace {
        template<typename T>
        void AssertEntityHasExpectedSize(size_t baseSize) {
            // Arrange:
            auto expectedSize = 
                    baseSize // base
                    + Key_Size // contract key
                    + sizeof(uint64_t); // batch id
            
            // Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 40u, sizeof(T));
        }

        template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_SynchronizationSingleTransaction, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
    }

    ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(SynchronizationSingle)

    // endregion

    // region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		SynchronizationSingleTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = SynchronizationSingleTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(SynchronizationSingleTransaction), realSize);
	}

	// endregion
}}