/**
*** FOR TRAINING PURPOSES ONLY
**/

#include "src/model/HelloTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

        using TransactionType = HelloTransaction;

#define TEST_CLASS HelloTransactionTests

        // region size + properties

        namespace {
            template<typename T>
            void AssertEntityHasExpectedSize(size_t baseSize) {
                // Arrange:
                auto expectedSize = baseSize // base
                                    + sizeof(uint16_t); // message count

                // Assert:
                EXPECT_EQ(expectedSize, sizeof(T));
                EXPECT_EQ(baseSize + 2u, sizeof(T));
            }

            template<typename T>
            void AssertTransactionHasExpectedProperties() {
                // Assert:
                EXPECT_EQ(Entity_Type_Hello, static_cast<EntityType>(T::Entity_Type));
                EXPECT_EQ(1, static_cast<VersionType>(T::Current_Version));
            }
        }

        // defined in tests/test/core/TransactionTestUtils.h
        // second parameter expands to HelloTransaction and EmbeddedHelloTransaction
        ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(Hello)

        // no need for DEFINE_DUAL_ATTACHMENT_POINTER_TESTS
        // this plugin has no attachment needed

        // basic gtest
        TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
            // Arrange:
            HelloTransaction transaction;
            test::SetMaxValue(transaction.MessageCount);
              // Act:
            auto realSize = HelloTransaction::CalculateRealSize(transaction);

            // Assert:
            EXPECT_EQ(sizeof(HelloTransaction), realSize);
        }

}}// end catapult::model

