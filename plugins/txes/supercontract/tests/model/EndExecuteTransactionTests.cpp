/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/EndExecuteTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = EndExecuteTransaction;

#define TEST_CLASS EndExecuteTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ sizeof(uint8_t) // mosaic count
					+ Hash256_Size // operation token
					+ sizeof(uint16_t); // result

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 35u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_EndExecute, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(EndExecute)

	// endregion

	// region data pointers

	namespace {
		struct EndExecuteTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t numMosaics) {
				uint32_t entitySize = sizeof(TransactionType) + numMosaics * sizeof(model::UnresolvedMosaic);
				auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->MosaicCount = numMosaics;
				return pTransaction;
			}

			template<typename TEntity>
			static auto GetAttachmentPointer(TEntity& entity) {
				return entity.MosaicsPtr();
			}
		};
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, EndExecuteTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;
		transaction.MosaicCount = 10;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TransactionType) + 10 * sizeof(model::UnresolvedMosaic), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		TransactionType transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.MosaicCount);

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TransactionType) + 0xFF * sizeof(model::UnresolvedMosaic), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
