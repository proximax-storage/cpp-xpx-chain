/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/utils/MemoryUtils.h"
#include "plugins/txes/contract_v2/src/model/DeployTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

    using TransactionType = DeployTransaction;

#define TEST_CLASS DeployTransactionTests

    // region size + properties

    namespace {
        template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
            // Arrange:
			auto expectedSize = baseSize // base
                    + Key_Size // drive key size
                    + sizeof(uint16_t) // file name size
                    + sizeof(uint16_t) // function name size
                    + sizeof(uint16_t) // actual arguments size
                    + sizeof(Amount) // SC units
                    + sizeof(Amount) // SM units
                    + sizeof(uint8_t) // additional tokens count
                    + 1 // single approvement
                    + sizeof(uint16_t) // .wasm file name size
                    + sizeof(uint16_t) // called function name size
                    + sizeof(Amount) // SC units limit
                    + sizeof(Amount) // SM units limit
                    + sizeof(uint32_t) // prepaid automated executions count
                    + Key_Size // public key for Supercontract Closure

            // Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 112u, sizeof(T));
        }

        template<typename T>
		void AssertTransactionHasExpectedProperties() {
            // Assert:
            EXPECT_EQ(Entity_Type_Deploy, static_cast<EntityType>(T::Entity_Type));
            EXPECT_EQ(1u, static_cast<VersionType>(T::Current_Version));
        }
    }

    ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(Deploy)

    // endregion

    // region data pointers

    namespace {
        struct DeployTransactionTraits {
            static auto GenerateEntityWithAttachments(uint8_t numServicePayment) {
                uint32_t entitySize = sizeof(DeployTransaction) + numServicePayment * sizeof(Mosaic);
                auto pTransaction = utils::MakeUniqueWithSize<DeployTransaction>(entitySize);
                pTransaction->Size = entitySize;
                pTransaction->ServicePaymentCount = numServicePayment;
                return pTransaction;
            }

			template<typename TEntity>
            static auto GetAttachmentPointer(TEntity& entity) {
				return entity.ServicePaymentPtr();
			}
        };
    }

    DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, DeployTransactionTraits)

    // endregion

    // region CalculateRealSize

    TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
        // Arrange:
        DeployTransaction transaction;
        transaction.Size = 0;
        constexpr auto fileNameSize = 3;
		transaction.FileNameSize = fileNameSize;
        constexpr auto functionNameSize = 5;
		transaction.FunctionNameSize = functionNameSize;
        constexpr auto actualArgumentsSize = 7;
		transaction.ActualArgumentsSize = actualArgumentsSize;
        constexpr auto automatedExecutionFileNameSize = 5;
		transaction.AutomatedExecutionFileNameSize = automatedExecutionFileNameSize;
        constexpr auto automatedExecutionFunctionNameSize = 3;
		transaction.AutomatedExecutionFunctionNameSize = automatedExecutionFunctionNameSize;

        // Act:
		auto realSize = DeployTransaction::CalculateRealSize(transaction);

        // Assert:
		EXPECT_EQ(sizeof(DeployTransaction) + fileNameSize + functionNameSize + actualArgumentsSize + automatedExecutionFileNameSize + automatedExecutionFunctionNameSize, realSize);
    }

    TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
        DeployTransaction transaction;
		test::SetMaxValue(transaction.Size);
        test::SetMaxValue(transaction.ServicePaymentCount);

        // Act:
		auto realSize = DeployTransaction::CalculateRealSize(transaction);

        // Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(DeployTransaction) + 0xFF * sizeof(Mosaic), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
    }

    // endregion
}}