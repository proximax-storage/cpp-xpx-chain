/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/CatapultConfigEntryMapper.h"
#include "tests/test/CatapultConfigMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS CatapultConfigEntryMapperTests

	// region ToDbModel

	namespace {
		state::CatapultConfigEntry CreateCatapultConfigEntry() {
			return state::CatapultConfigEntry(Height(), "aaa", "bbb");
		}
	}

	TEST(TEST_CLASS, CanMapCatapultConfigEntry_ModelToDbModel) {
		// Arrange:
		auto entry = CreateCatapultConfigEntry();

		// Act:
		auto document = ToDbModel(entry);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));

		auto catapultConfigView = documentView["catapultConfig"].get_document().view();
		EXPECT_EQ(3u, test::GetFieldCount(catapultConfigView));
		test::AssertEqualCatapultConfigData(entry, catapultConfigView);
	}

	// endregion

	// region ToCatapultConfigEntry

	namespace {
		bsoncxx::document::value CreateDbCatapultConfigEntry() {
			return ToDbModel(CreateCatapultConfigEntry());
		}
	}

	TEST(TEST_CLASS, CanMapCatapultConfigEntry_DbModelToModel) {
		// Arrange:
		auto dbCatapultConfigEntry = CreateDbCatapultConfigEntry();

		// Act:
		auto entry = ToCatapultConfigEntry(dbCatapultConfigEntry);

		// Assert:
		auto view = dbCatapultConfigEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));

		auto catapultConfigView = view["catapultConfig"].get_document().view();
		EXPECT_EQ(3u, test::GetFieldCount(catapultConfigView));
		test::AssertEqualCatapultConfigData(entry, catapultConfigView);
	}

	// endregion
}}}