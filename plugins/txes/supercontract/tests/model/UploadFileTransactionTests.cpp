/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/UploadFileTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = UploadFileTransaction;

#define TEST_CLASS UploadFileTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize // base
					+ Key_Size // drive key
					+ Hash256_Size // a new drive root hash
					+ Hash256_Size // Xor of a new drive root hash with previous root hash
					+ sizeof(uint16_t) // file add actions count
					+ sizeof(uint16_t); // file remove actions count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 100u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_UploadFile, static_cast<EntityType>(T::Entity_Type));
			EXPECT_EQ(1u, static_cast<VersionType>(T::Current_Version));
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(UploadFile)

	// endregion

	// region data pointers

	namespace {
		struct UploadFileTransactionTraits {
			static auto GenerateEntityWithAttachments(uint16_t addActionsCount, uint16_t removeActionsCount) {
				uint32_t entitySize = sizeof(TransactionType) + addActionsCount * sizeof(AddAction) + removeActionsCount * sizeof(RemoveAction);
				auto pTransaction = utils::MakeUniqueWithSize<TransactionType>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->AddActionsCount = addActionsCount;
				pTransaction->RemoveActionsCount = removeActionsCount;
				return pTransaction;
			}

			static constexpr size_t GetAttachment1Size(uint16_t addActionsCount) {
				return addActionsCount * sizeof(AddAction);
			}

			template<typename TEntity>
			static auto GetAttachmentPointer1(TEntity& entity) {
				return entity.AddActionsPtr();
			}

			template<typename TEntity>
			static auto GetAttachmentPointer2(TEntity& entity) {
				return entity.RemoveActionsPtr();
			}
		};
	}

	DEFINE_DUAL_ATTACHMENT_POINTER_TESTS(TEST_CLASS, UploadFileTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		TransactionType transaction;
		transaction.Size = 0;
		transaction.AddActionsCount = 2;
		transaction.RemoveActionsCount = 4;

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(TransactionType) + 2 * sizeof(AddAction) + 4 * sizeof(RemoveAction), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		TransactionType transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.AddActionsCount);
		test::SetMaxValue(transaction.RemoveActionsCount);

		// Act:
		auto realSize = TransactionType::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(TransactionType) + 0xFFFF * sizeof(AddAction) + 0xFFFF * sizeof(RemoveAction), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
