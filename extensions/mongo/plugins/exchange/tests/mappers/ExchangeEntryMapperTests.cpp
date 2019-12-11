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
		auto address = test::GenerateRandomByteArray<Address>();

		// Act:
		auto document = ToDbModel(entry, address);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));
		test::AssertEqualExchangeData(entry, address, documentView["exchange"].get_document().view());
	}

	// endregion

	// region ToExchangeEntry

	namespace {
		bsoncxx::document::value CreateDbExchangeEntry(const Address& address) {
			return ToDbModel(test::CreateExchangeEntry(), address);
		}
	}

	TEST(TEST_CLASS, CanMapExchangeEntry_DbModelToModel) {
		// Arrange:
		auto address = test::GenerateRandomByteArray<Address>();
		auto dbExchangeEntry = CreateDbExchangeEntry(address);

		// Act:
		auto entry = ToExchangeEntry(dbExchangeEntry);

		// Assert:
		auto view = dbExchangeEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));
		test::AssertEqualExchangeData(entry, address, view["exchange"].get_document().view());
	}

	// endregion
}}}