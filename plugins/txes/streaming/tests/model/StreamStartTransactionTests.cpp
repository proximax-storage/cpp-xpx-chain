/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/StreamStartTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = StreamStartTransaction;

#define TEST_CLASS StreamStartTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ Key_Size // drive key size
					+ sizeof(uint64_t) // expected upload size
					+ sizeof(uint16_t) // folderName size
					+ sizeof(Amount); // feedback fee

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 50u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_StreamStart, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(StreamStart)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		StreamStartTransaction transaction;
		transaction.Size = 0;
		constexpr auto folderNameSize = 3;
		transaction.FolderNameSize = folderNameSize;

		// Act:
		auto realSize = StreamStartTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(StreamStartTransaction) + folderNameSize, realSize);
	}

	// endregion
}}
