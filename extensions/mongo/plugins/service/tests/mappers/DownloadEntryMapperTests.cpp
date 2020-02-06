/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/DownloadEntryMapper.h"
#include "plugins/txes/service/tests/test/ServiceTestUtils.h"
#include "tests/test/ServiceMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	TEST(TEST_CLASS, CanMapDownloadEntry_ModelToDbModel) {
		// Arrange:
		auto entry = test::CreateDownloadEntry();
		auto address = test::GenerateRandomByteArray<Address>();

		// Act:
		auto document = ToDbModel(entry, address);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));
		test::AssertEqualDownloadData(entry, address, documentView["downloadInfo"].get_document().view());
	}

	// endregion

	// region ToDownloadEntry

	namespace {
		bsoncxx::document::value CreateDbDownloadEntry(const Address& address) {
			return ToDbModel(test::CreateDownloadEntry(), address);
		}
	}

	TEST(TEST_CLASS, CanMapDownloadEntry_DbModelToModel) {
		// Arrange:
		auto address = test::GenerateRandomByteArray<Address>();
		auto dbDownloadEntry = CreateDbDownloadEntry(address);

		// Act:
		auto entry = ToDownloadEntry(dbDownloadEntry);

		// Assert:
		auto view = dbDownloadEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));
		test::AssertEqualDownloadData(entry, address, view["downloadInfo"].get_document().view());
	}

	// endregion
}}}