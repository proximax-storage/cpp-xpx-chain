/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/EndDriveVerificationTransaction.h"
#include "catapult/utils/MemoryUtils.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/test/core/VariableSizedEntityTestUtils.h"
#include "tests/test/nodeps/NumericTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace model {

#define TEST_CLASS EndDriveVerificationTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize // base
					+ sizeof(uint16_t); // failure count

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 2u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_End_Drive_Verification, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(EndDriveVerification)

	// endregion

	// region data pointers

	namespace {
		struct EndDriveVerificationTransactionTraits {
			static auto GenerateEntityWithAttachments(uint8_t failureCount) {
				uint32_t entitySize = sizeof(EndDriveVerificationTransaction) + failureCount * sizeof(VerificationFailure);
				auto pTransaction = utils::MakeUniqueWithSize<EndDriveVerificationTransaction>(entitySize);
				pTransaction->Size = entitySize;
				pTransaction->FailureCount = failureCount;
				return pTransaction;
			}

			template<typename TEntity>
			static auto GetAttachmentPointer(TEntity& entity) {
				return entity.FailuresPtr();
			}
		};
	}

	DEFINE_ATTACHMENT_POINTER_TESTS(TEST_CLASS, EndDriveVerificationTransactionTraits)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		EndDriveVerificationTransaction transaction;
		transaction.Size = 0;
		transaction.FailureCount = 10;

		// Act:
		auto realSize = EndDriveVerificationTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(EndDriveVerificationTransaction) + 10 * sizeof(VerificationFailure), realSize);
	}

	TEST(TEST_CLASS, CalculateRealSizeDoesNotOverflowWithMaxValues) {
		// Arrange:
		EndDriveVerificationTransaction transaction;
		test::SetMaxValue(transaction.Size);
		test::SetMaxValue(transaction.FailureCount);

		// Act:
		auto realSize = EndDriveVerificationTransaction::CalculateRealSize(transaction);

		// Assert:
		ASSERT_EQ(0xFFFFFFFF, transaction.Size);
		EXPECT_EQ(sizeof(EndDriveVerificationTransaction) + 0xFFFF * sizeof(VerificationFailure), realSize);
		EXPECT_GT(0xFFFFFFFF, realSize);
	}

	// endregion
}}
