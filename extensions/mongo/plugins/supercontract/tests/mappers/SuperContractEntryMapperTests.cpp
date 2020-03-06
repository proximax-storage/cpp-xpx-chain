/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/SuperContractEntryMapper.h"
#include "plugins/txes/supercontract/tests/test/SuperContractTestUtils.h"
#include "tests/test/SuperContractMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

	// region ToDbModel

	TEST(TEST_CLASS, CanMapSuperContractEntry_ModelToDbModel) {
		// Arrange:
		auto entry = test::CreateSuperContractEntry();
		auto address = test::GenerateRandomByteArray<Address>();

		// Act:
		auto document = ToDbModel(entry, address);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));
		test::AssertEqualMongoSuperContractData(entry, address, documentView["supercontract"].get_document().view());
	}

	// endregion

	// region ToSuperContractEntry

	namespace {
		bsoncxx::document::value CreateDbSuperContractEntry(const Address& address) {
			return ToDbModel(test::CreateSuperContractEntry(), address);
		}
	}

	TEST(TEST_CLASS, CanMapSuperContractEntry_DbModelToModel) {
		// Arrange:
		auto address = test::GenerateRandomByteArray<Address>();
		auto dbSuperContractEntry = CreateDbSuperContractEntry(address);

		// Act:
		auto entry = ToSuperContractEntry(dbSuperContractEntry);

		// Assert:
		auto view = dbSuperContractEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));
		test::AssertEqualMongoSuperContractData(entry, address, view["supercontract"].get_document().view());
	}

	// endregion
}}}