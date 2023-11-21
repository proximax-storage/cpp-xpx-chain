/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/SdaExchangeEntryMapper.h"
#include "plugins/txes/exchange_sda/tests/test/SdaExchangeTestUtils.h"
#include "tests/test/SdaExchangeMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

    // region ToDbModel

    TEST(TEST_CLASS, CanMapSdaExchangeEntry_ModelToDbModel) {
        // Arrange:
		auto entry = test::CreateSdaExchangeEntry();
		auto address = test::GenerateRandomByteArray<Address>();

        // Act:
		auto document = ToDbModel(entry, address);
		auto documentView = document.view();

        // Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));
		test::AssertEqualSdaExchangeData(entry, address, documentView["exchangesda"].get_document().view());
    }

    // endregion

	// region ToSdaExchangeEntry

    namespace {
		bsoncxx::document::value CreateDbSdaExchangeEntry(const Address& address) {
			return ToDbModel(test::CreateSdaExchangeEntry(), address);
		}
	}

    TEST(TEST_CLASS, CanMapSdaExchangeEntry_DbModelToModel) {
		// Arrange:
		auto address = test::GenerateRandomByteArray<Address>();
		auto dbSdaExchangeEntry = CreateDbSdaExchangeEntry(address);

		// Act:
		auto entry = ToSdaExchangeEntry(dbSdaExchangeEntry);

		// Assert:
		auto view = dbSdaExchangeEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));
		test::AssertEqualSdaExchangeData(entry, address, view["exchangesda"].get_document().view());
	}

	// endregion
}}}