/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/DownloadTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = DownloadTransaction;

#define TEST_CLASS DownloadTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ sizeof(Key) // drive key
					+ sizeof(uint64_t) // prepaid download size
					+ sizeof(Amount) // feedback fee size
					+ sizeof(uint16_t); // public keys size

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 50u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_Download, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(Download)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		DownloadTransaction transaction;
		transaction.Size = 0;
		transaction.ListOfPublicKeysSize = test::Random16();

		// Act:
		auto realSize = DownloadTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(DownloadTransaction) + transaction.ListOfPublicKeysSize * Key_Size, realSize);
	}

	// endregion
}}
