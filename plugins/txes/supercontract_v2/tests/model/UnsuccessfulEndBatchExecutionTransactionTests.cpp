/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/UnsuccessfulEndBatchExecutionTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

    using TransactionType = UnsuccessfulEndBatchExecutionTransaction;

#define TEST_CLASS UnsuccessfulEndBatchExecutionTransactionTests

    // region size + properties

    namespace {
        template<typename T>
        void AssertEntityHasExpectedSize(size_t baseSize) {
            // Arrange:
            auto expectedSize = 
                    baseSize // base
                    + Key_Size // contract key
                    + sizeof(uint64_t) // batch id
                    + sizeof(uint64_t) // check next block for automatic executions
                    + sizeof(uint16_t) // number of cosigners
                    + sizeof(uint16_t) // number of calls
            
            // Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 52u, sizeof(T));
        }

        template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_UnsuccessfulEndBatchExecutionTransaction, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
    }

    ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(UnsuccessfulEndBatchExecution)

    // endregion

    // region data pointers

    namespace {
		struct UnsuccessfulEndBatchExecutionTransactionTraits {
            static auto GenerateEntityWithAttachments(uint16_t cosignersNumber, uint16_t callsNumber) {
                uint32_t entitySize = sizeof(TransactionType) 
                            + cosignersNumber * sizeof(Key) 
                            + cosignersNumber * sizeof(Signature) 
                            + cosignersNumber * sizeof(RawProofOfExecution) 
                            + callsNumber * sizeof(ShortCallDigest)
                            + static_cast<uint64_t>(cosignersNumber) * static_cast<uint64_t>(callsNumber) * sizeof(CallPayment);
                auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->CosignersNumber = cosignersNumber;
				pTransaction->CallsNumber = callsNumber;
				return pTransaction;
            }

            static constexpr size_t GetAttachment1Size(uint16_t cosignersNumber) {
				return cosignersNumber;
			}

            template<typename TEntity>
			static auto GetAttachmentPointer(TEntity& entity) {
				return entity.PublicKeysPtr();
			}

            template<typename TEntity>
			static auto GetAttachmentPointer1(TEntity& entity) {
				return entity.SignaturesPtr();
			}

            template<typename TEntity>
			static auto GetAttachmentPointer2(TEntity& entity) {
				return entity.ProofsOfExecutionPtr();
			}

            template<typename TEntity>
			static auto GetAttachmentPointer3(TEntity& entity) {
				return entity.CallDigestsPtr();
			}
        };
    }
    // endregion

    // region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		UnsuccessfulEndBatchExecutionTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = UnsuccessfulEndBatchExecutionTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(UnsuccessfulEndBatchExecutionTransaction), realSize);
	}

	// endregion
}}