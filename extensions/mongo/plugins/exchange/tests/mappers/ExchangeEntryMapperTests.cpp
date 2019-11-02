/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/ExchangeEntryMapper.h"
#include "plugins/txes/exchange/tests/test/ExchangeTestUtils.h"
#include "tests/test/ExchangeMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	TEST(TEST_CLASS, CanMapExchangeEntry_ModelToDbModel) {
		// Arrange:
		auto entry = test::CreateExchangeEntry();

		// Act:
		auto document = ToDbModel(entry);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));
		test::AssertEqualExchangeData(entry, documentView["exchange"].get_document().view());
	}

	// endregion

	// region ToExchangeEntry

	namespace {
		bsoncxx::document::value CreateDbExchangeEntry() {
			return ToDbModel(test::CreateExchangeEntry());
		}
	}

	TEST(TEST_CLASS, CanMapExchangeEntry_DbModelToModel) {
		// Arrange:
		auto dbExchangeEntry = CreateDbExchangeEntry();

		// Act:
		auto entry = ToExchangeEntry(dbExchangeEntry);

		// Assert:
		auto view = dbExchangeEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));
		test::AssertEqualExchangeData(entry, view["exchange"].get_document().view());
	}

	// endregion
}}}