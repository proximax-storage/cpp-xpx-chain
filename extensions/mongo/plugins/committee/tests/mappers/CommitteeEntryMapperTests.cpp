/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "plugins/txes/committee/tests/test/CommitteeTestUtils.h"
#include "src/mappers/CommitteeEntryMapper.h"
#include "tests/test/CommitteeMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS CommitteeEntryMapperTests

	// region ToDbModel

	TEST(TEST_CLASS, CanMapCommitteeEntry_ModelToDbModel) {
		// Arrange:
		auto entry = test::CreateCommitteeEntry();

		// Act:
		auto document = ToDbModel(entry);
		auto view = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(view));
		test::AssertEqualCommitteeData(entry, view["harvester"].get_document().view());
	}

	// endregion

	// region ToCommitteeEntry

	namespace {
		bsoncxx::document::value CreateDbCommitteeEntry() {
			return ToDbModel(test::CreateCommitteeEntry());
		}
	}

	TEST(TEST_CLASS, CanMapCommitteeEntry_DbModelToModel) {
		// Arrange:
		auto dbCommitteeEntry = CreateDbCommitteeEntry();

		// Act:
		auto entry = ToCommitteeEntry(dbCommitteeEntry);

		// Assert:
		auto view = dbCommitteeEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));
		test::AssertEqualCommitteeData(entry, view["harvester"].get_document().view());
	}

	// endregion
}}}