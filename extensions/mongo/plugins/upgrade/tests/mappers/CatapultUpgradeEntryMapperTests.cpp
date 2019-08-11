/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/CatapultUpgradeEntryMapper.h"
#include "tests/test/CatapultUpgradeMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS CatapultUpgradeEntryMapperTests

	// region ToDbModel

	namespace {
		state::CatapultUpgradeEntry CreateCatapultUpgradeEntry() {
			return state::CatapultUpgradeEntry(Height(), CatapultVersion());
		}
	}

	TEST(TEST_CLASS, CanMapCatapultUpgradeEntry_ModelToDbModel) {
		// Arrange:
		auto entry = CreateCatapultUpgradeEntry();

		// Act:
		auto document = ToDbModel(entry);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));

		auto catapultUpgradeView = documentView["catapultUpgrade"].get_document().view();
		EXPECT_EQ(2u, test::GetFieldCount(catapultUpgradeView));
		test::AssertEqualCatapultUpgradeData(entry, catapultUpgradeView);
	}

	// endregion

	// region ToCatapultUpgradeEntry

	namespace {
		bsoncxx::document::value CreateDbCatapultUpgradeEntry() {
			return ToDbModel(CreateCatapultUpgradeEntry());
		}
	}

	TEST(TEST_CLASS, CanMapCatapultUpgradeEntry_DbModelToModel) {
		// Arrange:
		auto dbCatapultUpgradeEntry = CreateDbCatapultUpgradeEntry();

		// Act:
		auto entry = ToCatapultUpgradeEntry(dbCatapultUpgradeEntry);

		// Assert:
		auto view = dbCatapultUpgradeEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));

		auto catapultUpgradeView = view["catapultUpgrade"].get_document().view();
		EXPECT_EQ(2u, test::GetFieldCount(catapultUpgradeView));
		test::AssertEqualCatapultUpgradeData(entry, catapultUpgradeView);
	}

	// endregion
}}}