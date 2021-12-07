/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/DataModificationApprovalTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = DataModificationApprovalTransaction;

#define TEST_CLASS DataModificationApprovalTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ Key_Size // drive key
					+ Hash256_Size // data modification id
					+ Hash256_Size // file structure CDI
					+ sizeof(uint64_t) // file structure size
					+ sizeof(uint64_t) // metafiles size
					+ sizeof(uint64_t) // used drive size
					+ sizeof(uint8_t) // opinion count
					+ sizeof(uint8_t) // judging keys count
					+ sizeof(uint8_t) // overlapping keys count
					+ sizeof(uint8_t) // judged keys count
					+ sizeof(uint8_t); // opinion element count
			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 125u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_DataModificationApproval, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(DataModificationApproval)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		DataModificationApprovalTransaction transaction;
		transaction.JudgingKeysCount = 0;
		transaction.OverlappingKeysCount = 0;
		transaction.JudgedKeysCount = 0;
		transaction.OpinionCount = 0;
		transaction.OpinionElementCount = 0;
		transaction.Size = 0;

		// Act:
		auto realSize = DataModificationApprovalTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(DataModificationApprovalTransaction), realSize);
	}

	// endregion
}}
