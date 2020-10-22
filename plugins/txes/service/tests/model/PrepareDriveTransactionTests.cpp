/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/PrepareDriveTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"

namespace catapult { namespace model {

	using TransactionType = PrepareDriveTransaction;

#define TEST_CLASS PrepareDriveTransactionTests

	// region size + properties

	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize = baseSize // base
					+ Key_Size // drive owner key
					+ sizeof(uint64_t) // drive duration
					+ sizeof(uint64_t) // billing period
					+ sizeof(uint64_t) // billing price
					+ sizeof(uint64_t) // drive size
					+ sizeof(uint16_t) // replicas
					+ sizeof(uint16_t) // min replicas
					+ sizeof(uint8_t); // percent of approvers

			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 69u, sizeof(T));
		}

		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_PrepareDrive, static_cast<EntityType>(T::Entity_Type));
			EXPECT_EQ(3u, static_cast<VersionType>(T::Current_Version));
		}
	}

	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(PrepareDrive)

	// endregion
}}
