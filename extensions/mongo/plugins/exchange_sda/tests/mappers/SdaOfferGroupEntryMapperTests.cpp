/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/SdaOfferGroupEntryMapper.h"
#include "plugins/txes/exchange_sda/tests/test/SdaExchangeTestUtils.h"
#include "tests/test/SdaExchangeMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

    // region ToDbModel

    TEST(TEST_CLASS, CanMapSdaOfferGroupEntry_ModelToDbModel) {
        // Arrange:
        auto entry = test::CreateSdaOfferGroupEntry();

        // Act:
        auto document = ToDbModel(entry);
        auto documentView = document.view();

        // Assert:
        EXPECT_EQ(1u, test::GetFieldCount(documentView));
        test::AssertEqualSdaOfferGroupData(entry, documentView["sdaoffergroups"].get_document().view());
    }

    // endregion

    // region ToSdaOfferGroupEntry

    namespace {
        bsoncxx::document::value CreateDbSdaOfferGroupEntry() {
            return ToDbModel(test::CreateSdaOfferGroupEntry());
        }
    }

    TEST(TEST_CLASS, CanMapSdaOfferGroupEntry_DbModelToModel) {
        // Arrange:
        auto dbSdaOfferGroupEntry = CreateDbSdaOfferGroupEntry();

        // Act:
        auto entry = ToSdaOfferGroupEntry(dbSdaOfferGroupEntry);

        // Assert:
        auto view = dbSdaOfferGroupEntry.view();
        EXPECT_EQ(1u, test::GetFieldCount(view));
        test::AssertEqualSdaOfferGroupData(entry, view["sdaoffergroups"].get_document().view());
    }

    // endregion
}}}