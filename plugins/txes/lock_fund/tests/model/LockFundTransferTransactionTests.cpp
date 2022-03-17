/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/LockFundTransferTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS LockFundTransferTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ sizeof(uint64_t) // duration
					+ sizeof(uint8_t) // action
					+ sizeof(uint8_t); // mosaics count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 10u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_Lock_Fund_Transfer, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(LockFundTransfer)

	// endregion

	// region data pointers

	namespace {
		struct LockFundTransferTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t numMosaics) {
				uint32_t entitySize = sizeof(LockFundTransferTransaction) + numMosaics * sizeof(Mosaic);
				auto pTransaction = utils::MakeUniqueWithSize<LockFundTransferTransaction>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->Duration = BlockDuration(200000u);
				pTransaction->Action = model::LockFundAction::Lock;
				pTransaction->MosaicsCount = numMosaics;
				return pTransaction;
			}
			template<typename TEntity>
			static auto GetAttachmentPointer(TEntity& entity) {
				return entity.MosaicsPtr();
			}
		};
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, LockFundTransferTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		LockFundTransferTransaction transaction;
		transaction.Size = 0;
		transaction.Action = LockFundAction::Lock;
		transaction.Duration = BlockDuration(200000);
		transaction.MosaicsCount = 7;

		// Act:
		auto realSize = LockFundTransferTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(16u, sizeof(Mosaic));
		EXPECT_EQ(sizeof(LockFundTransferTransaction) + 7 * sizeof(Mosaic), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		LockFundTransferTransaction transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.Duration);
			test::SetMaxValue(transaction.Action);
		test::SetMaxValue(transaction.MosaicsCount);

		// Act:
		auto realSize = LockFundTransferTransaction::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(LockFundTransferTransaction) + 0xFF * sizeof(Mosaic), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
