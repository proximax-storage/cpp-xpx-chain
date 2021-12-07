/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/EndDriveVerificationTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

        using TransactionType = EndDriveVerificationTransaction;

#define TEST_CLASS EndDriveVerificationTransactionTests

        // region size + properties

        namespace {
            template<typename T>
            void AssertEntityHasExpectedSize(size_t baseSize) {
                // Arrange:
                auto expectedSize =
                        baseSize // base
                        + Key_Size // drive key size
                        + Hash256_Size // verification trigger hash size
                        + sizeof(uint16_t) // count of provers
                        + sizeof(uint16_t); // count of opinions

                // Assert:
                EXPECT_EQ(expectedSize, sizeof(T));
                EXPECT_EQ(baseSize + 68u, sizeof(T));
            }

            template<typename T>
            void AssertTransactionHasExpectedProperties() {
                // Assert:
                EXPECT_EQ(Entity_Type_EndDriveVerification, T::Entity_Type);
                EXPECT_EQ(1u, T::Current_Version);
            }
        }

        ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(EndDriveVerification)

        // endregion

        // region CalculateRealSize

        TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
            // Arrange:
			EndDriveVerificationTransaction transaction;
			transaction.ProversCount = 0;
			transaction.VerificationOpinionsCount = 0;
			transaction.Size = 0;

            // Act:
            auto realSize = EndDriveVerificationTransaction::CalculateRealSize(transaction);

            // Assert:
            EXPECT_EQ(sizeof(EndDriveVerificationTransaction), realSize);
        }

        // endregion
    }}
