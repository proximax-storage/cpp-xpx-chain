/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/StartExecuteTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

	using TransactionType = StartExecuteTransaction;

#define TEST_CLASS StartExecuteTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ sizeof(uint8_t) // mosaic count
					+ Key_Size // super contract
					+ sizeof(uint8_t) // function size
					+ sizeof(uint16_t); // data size

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 36u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_StartExecute, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(StartExecute)

	// endregion

	// region data pointers

	namespace {
		struct StartExecuteTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t functionSize, uint8_t numMosaics, uint16_t dataSize) {
				uint32_t entitySize = sizeof(TransactionType) + numMosaics * sizeof(model::UnresolvedMosaic) + functionSize + dataSize;
				auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->MosaicCount = numMosaics;
				pTransaction->FunctionSize = functionSize;
				pTransaction->DataSize = dataSize;
				return pTransaction;
			}

			static constexpr size_t GetAttachment1Size(uint8_t functionSize) {
				return functionSize;
			}

			static constexpr size_t GetAttachment2Size(uint8_t numMosaics) {
				return numMosaics * sizeof(model::UnresolvedMosaic);
			}

			template<typename TEntity>
			static auto GetAttachmentPointer1(TEntity& entity) {
				return entity.FunctionPtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer2(TEntity& entity) {
				return entity.MosaicsPtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer3(TEntity& entity) {
				return entity.DataPtr();
			}
		};
	}

	DEFINE_TRIPLE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, StartExecuteTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;
		transaction.MosaicCount = 10;
		transaction.FunctionSize = 20;
		transaction.DataSize = 30;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TransactionType) + 10 * sizeof(model::UnresolvedMosaic) + 20  + 30, realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		TransactionType transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.MosaicCount);
		test::SetMaxValue(transaction.FunctionSize);
		test::SetMaxValue(transaction.DataSize);

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TransactionType) + 0xFF * sizeof(model::UnresolvedMosaic) + 0xFF + 0xFFFF, realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
