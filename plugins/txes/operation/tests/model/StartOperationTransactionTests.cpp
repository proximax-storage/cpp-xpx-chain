/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/StartOperationTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

	using TransactionType = StartOperationTransaction;

#define TEST_CLASS StartOperationTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ sizeof(uint8_t) // mosaic count
					+ sizeof(BlockDuration) // duration
					+ sizeof(uint8_t); // executor count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 10u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_StartOperation, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(StartOperation)

	// endregion

	// region data pointers

	namespace {
		struct StartOperationTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t numMosaics, uint8_t numExecutors) {
				uint32_t entitySize = sizeof(TransactionType) + numMosaics * sizeof(model::UnresolvedMosaic) + numExecutors * Key_Size;
				auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->MosaicCount = numMosaics;
				pTransaction->ExecutorCount = numExecutors;
				return pTransaction;
			}

			static constexpr size_t GetAttachment1Size(uint8_t numMosaics) {
				return numMosaics * sizeof(model::UnresolvedMosaic);
			}

			template<typename TEntity>
			static auto GetAttachmentPointer1(TEntity& entity) {
				return entity.MosaicsPtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer2(TEntity& entity) {
				return entity.ExecutorsPtr();
			}
		};
	}

	DEFINE_DUAL_ATTACHMENT_POINTER_TESTS(TEST_CLASS, StartOperationTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;
		transaction.MosaicCount = 10;
		transaction.ExecutorCount = 20;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TransactionType) + 10 * sizeof(model::UnresolvedMosaic) + 20 * Key_Size, realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		TransactionType transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.MosaicCount);
		test::SetMaxValue(transaction.ExecutorCount);

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TransactionType) + 0xFF * sizeof(model::UnresolvedMosaic) + 0xFF * Key_Size, realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
