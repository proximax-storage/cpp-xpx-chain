/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/DriveEntryMapper.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"
#include "tests/test/ServiceMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	TEST(TEST_CLASS, CanMapDriveEntry_ModelToDbModel) {
		// Arrange:
		auto entry = test::CreateDriveEntry();
		auto address = test::GenerateRandomByteArray<Address>();

		// Act:
		auto document = ToDbModel(entry, address);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));
		test::AssertEqualDriveData(entry, address, documentView["drive"].get_document().view());
	}

	// endregion

	// region ToDriveEntry

	namespace {
		bsoncxx::document::value CreateDbDriveEntry(const Address& address) {
			return ToDbModel(test::CreateDriveEntry(), address);
		}
	}

	TEST(TEST_CLASS, CanMapDriveEntry_DbModelToModel) {
		// Arrange:
		auto address = test::GenerateRandomByteArray<Address>();
		auto dbDriveEntry = CreateDbDriveEntry(address);

		// Act:
		auto entry = ToDriveEntry(dbDriveEntry);

		// Assert:
		auto view = dbDriveEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));
		test::AssertEqualDriveData(entry, address, view["drive"].get_document().view());
	}

	// endregion
}}}