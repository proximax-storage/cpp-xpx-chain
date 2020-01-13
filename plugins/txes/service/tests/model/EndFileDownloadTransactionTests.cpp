/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/EndFileDownloadTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS EndFileDownloadTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize // base
					+ Key_Size // file recipient key
					+ sizeof(uint16_t); // file count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 34u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_EndFileDownload, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(EndFileDownload)

	// endregion

	// region data pointers

	namespace {
		struct EndFileDownloadTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t fileCount) {
				uint32_t entitySize = sizeof(EndFileDownloadTransaction) + fileCount * sizeof(File);
				auto pTransaction = utils::MakeUniqueWithSize<EndFileDownloadTransaction>(entitySize);
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

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, EndFileDownloadTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		EndFileDownloadTransaction transaction;
		transaction.Size = 0;
		transaction.FileCount = 5;

		// Act:
		auto realSize = EndFileDownloadTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(EndFileDownloadTransaction) + 5 * sizeof(File), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		EndFileDownloadTransaction transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.FileCount);

		// Act:
		auto realSize = EndFileDownloadTransaction::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(EndFileDownloadTransaction) + 0xFFFF * sizeof(File), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
