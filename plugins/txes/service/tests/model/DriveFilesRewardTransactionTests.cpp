/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/DriveFilesRewardTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS DriveFilesRewardTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize // base
					+ sizeof(uint16_t); // upload info count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 2u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_DriveFilesReward, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(DriveFilesReward)

	// endregion

	// region data pointers

	namespace {
		struct DriveFilesRewardTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t uploadInfosCount) {
				uint32_t entitySize = sizeof(DriveFilesRewardTransaction) + uploadInfosCount * sizeof(UploadInfo);
				auto pTransaction = utils::MakeUniqueWithSize<DriveFilesRewardTransaction>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->UploadInfosCount = uploadInfosCount;
				return pTransaction;
			}

			template<typename TEntity>
			static auto GetAttachmentPointer(TEntity& entity) {
				return entity.UploadInfosPtr();
			}
		};
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, DriveFilesRewardTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		DriveFilesRewardTransaction transaction;
		transaction.Size = 0;
		transaction.UploadInfosCount = 10;

		// Act:
		auto realSize = DriveFilesRewardTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(DriveFilesRewardTransaction) + 10 * sizeof(UploadInfo), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		DriveFilesRewardTransaction transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.UploadInfosCount);

		// Act:
		auto realSize = DriveFilesRewardTransaction::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(DriveFilesRewardTransaction) + 0xFFFF * sizeof(UploadInfo), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
