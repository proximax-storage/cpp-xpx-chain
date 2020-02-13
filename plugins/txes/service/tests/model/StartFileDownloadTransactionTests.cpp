/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/StartFileDownloadTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS StartFileDownloadTransactionTests

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
			EXPECT_EQ(Entity_Type_StartFileDownload, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(StartFileDownload)

	// endregion

	// region data pointers

	namespace {
		struct StartFileDownloadTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t fileCount) {
				uint32_t entitySize = sizeof(StartFileDownloadTransaction) + fileCount * sizeof(DownloadAction);
				auto pTransaction = utils::MakeUniqueWithSize<StartFileDownloadTransaction>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->FileCount = fileCount;
				return pTransaction;
			}

			template<typename TEntity>
			static auto GetAttachmentPointer(TEntity& entity) {
				return entity.FilesPtr();
			}
		};
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, StartFileDownloadTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		StartFileDownloadTransaction transaction;
		transaction.Size = 0;
		transaction.FileCount = 5;

		// Act:
		auto realSize = StartFileDownloadTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(StartFileDownloadTransaction) + 5 * sizeof(DownloadAction), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		StartFileDownloadTransaction transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.FileCount);

		// Act:
		auto realSize = StartFileDownloadTransaction::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(StartFileDownloadTransaction) + 0xFFFF * sizeof(DownloadAction), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
