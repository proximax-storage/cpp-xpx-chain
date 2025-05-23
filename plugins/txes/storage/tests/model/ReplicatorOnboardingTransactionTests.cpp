/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/ReplicatorOnboardingTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = ReplicatorOnboardingTransaction;

#define TEST_CLASS ReplicatorOnboardingTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
					baseSize // base
					+ sizeof(uint64_t) // replicator capacity
					+ Key_Size // node boot key
					+ Hash256_Size // message
					+ Signature_Size; // message signature

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 136u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_ReplicatorOnboarding, T::Entity_Type);
			EXPECT_EQ(2u, T::Current_Version);
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(ReplicatorOnboarding)

	// endregion

	// region CalculateRealSize

	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		ReplicatorOnboardingTransaction transaction;
		transaction.Size = 0;

		// Act:
		auto realSize = ReplicatorOnboardingTransaction::CalculateRealSize(transaction);

		// Assert:
		EXPECT_EQ(sizeof(ReplicatorOnboardingTransaction), realSize);
	}

	// endregion
}}
