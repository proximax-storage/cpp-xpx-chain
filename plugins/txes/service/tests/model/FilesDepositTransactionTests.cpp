/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/FilesDepositTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS FilesDepositTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize // base
					+ Key_Size // drive key
					+ sizeof(uint16_t); // file count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 34u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_FilesDeposit, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(FilesDeposit)

	// endregion

	// region data pointers

	namespace {
		struct FilesDepositTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t filesCount) {
				uint32_t entitySize = sizeof(FilesDepositTransaction) + filesCount * sizeof(File);
				auto pTransaction = utils::MakeUniqueWithSize<FilesDepositTransaction>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->FilesCount = filesCount;
				return pTransaction;
			}

			template<typename TEntity>
			static auto GetAttachmentPointer(TEntity& entity) {
				return entity.FilesPtr();
			}
		};
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, FilesDepositTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		FilesDepositTransaction transaction;
		transaction.Size = 0;
		transaction.FilesCount = 5;

		// Act:
		auto realSize = FilesDepositTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(FilesDepositTransaction) + 5 * sizeof(File), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		FilesDepositTransaction transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.FilesCount);

		// Act:
		auto realSize = FilesDepositTransaction::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(FilesDepositTransaction) + 0xFFFF * sizeof(File), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
