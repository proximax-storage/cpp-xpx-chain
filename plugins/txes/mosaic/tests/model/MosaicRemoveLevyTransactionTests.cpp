/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#include "src/model/MosaicRemoveLevyTransaction.h"
#include "tests/test/core/TransactionTestUtils.h"
#include "tests/TestHarness.h"
#include "src/model/MosaicLevy.h"

namespace catapult { namespace model {

#define TEST_CLASS MosaicRemoveLevyTransactionTests
	
	// region size + properties
	
	namespace {
		template<typename T>
		void AssertEntityHasExpectedSize(size_t baseSize) {
			// Arrange:
			auto expectedSize =
				baseSize // base
				+ sizeof(MosaicId);     // mosaic id
				
			// Assert:
			EXPECT_EQ(expectedSize, sizeof(T));
			EXPECT_EQ(baseSize + 8, sizeof(T));
		}
		
		template<typename T>
		void AssertTransactionHasExpectedProperties() {
			// Assert:
			EXPECT_EQ(Entity_Type_Mosaic_Remove_Levy, T::Entity_Type);
			EXPECT_EQ(1u, T::Current_Version);
		}
	}
	
	ADD_BASIC_TRANSACTION_SIZE_PROPERTY_TESTS(MosaicRemoveLevy)
	
	// endregion
	
	TEST(TEST_CLASS, CanCalculateRealSizeWithReasonableValues) {
		// Arrange:
		MosaicRemoveLevyTransaction transaction;
		transaction.Size = 0;
		
		// Act:
		auto realSize = MosaicRemoveLevyTransaction::CalculateRealSize(transaction);
		
		// Assert:
		EXPECT_EQ(sizeof(MosaicRemoveLevyTransaction), realSize);
	}
}}
