/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/BlockchainUpgradeEntryMapper.h"
#include "tests/test/BlockchainUpgradeMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS BlockchainUpgradeEntryMapperTests

	// region ToDbModel

	namespace {
		state::BlockchainUpgradeEntry CreateBlockchainUpgradeEntry() {
			return state::BlockchainUpgradeEntry(Height(), BlockchainVersion());
		}
	}

	TEST(TEST_CLASS, CanMapBlockchainUpgradeEntry_ModelToDbModel) {
		// Arrange:
		auto entry = CreateBlockchainUpgradeEntry();

		// Act:
		auto document = ToDbModel(entry);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));

		auto blockchainUpgradeView = documentView["blockchainUpgrade"].get_document().view();
		EXPECT_EQ(2u, test::GetFieldCount(blockchainUpgradeView));
		test::AssertEqualBlockchainUpgradeData(entry, blockchainUpgradeView);
	}

	// endregion

	// region ToBlockchainUpgradeEntry

	namespace {
		bsoncxx::document::value CreateDbBlockchainUpgradeEntry() {
			return ToDbModel(CreateBlockchainUpgradeEntry());
		}
	}

	TEST(TEST_CLASS, CanMapBlockchainUpgradeEntry_DbModelToModel) {
		// Arrange:
		auto dbBlockchainUpgradeEntry = CreateDbBlockchainUpgradeEntry();

		// Act:
		auto entry = ToBlockchainUpgradeEntry(dbBlockchainUpgradeEntry);

		// Assert:
		auto view = dbBlockchainUpgradeEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));

		auto blockchainUpgradeView = view["blockchainUpgrade"].get_document().view();
		EXPECT_EQ(2u, test::GetFieldCount(blockchainUpgradeView));
		test::AssertEqualBlockchainUpgradeData(entry, blockchainUpgradeView);
	}

	// endregion
}}}