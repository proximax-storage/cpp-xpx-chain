/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/LevyEntry.h"
#include "tests/test/LevyTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS LevyEntryTests
	
	// region ctor

	TEST(TEST_CLASS, CanCreateLevyEntry) {
		// Arrange:
		auto levy = test::CreateValidMosaicLevy();
		auto pLevy = std::make_shared<state::LevyEntryData>(levy);
		
		// Act:
		auto entry = LevyEntry(MosaicId(225), std::move(pLevy));
		
		// Assert:
		EXPECT_EQ(MosaicId(225), entry.mosaicId());
		EXPECT_EQ(levy.Type, entry.levy()->Type);
		EXPECT_EQ(levy.MosaicId, entry.levy()->MosaicId);
		EXPECT_EQ(levy.Recipient, entry.levy()->Recipient);
		EXPECT_EQ(levy.Fee, entry.levy()->Fee);
	}
	
	TEST(TEST_CLASS, AssertNullRemoveEmpty) {
		// Arrange:
		auto entry = LevyEntry(MosaicId(225), nullptr);
		
		// Assert:
		EXPECT_THROW(entry.remove(Height(1)), catapult_runtime_error);
	}
	
	// endregion
}}
