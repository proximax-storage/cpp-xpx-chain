/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/ContractEntryMapper.h"
#include "tests/test/ContractMapperTestUtils.h"
#include "tests/test/ReputationTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS ContractEntryMapperTests

	// region ToDbModel

	namespace {
		state::ContractEntry CreateContractEntry() {
			state::ContractEntry entry(test::GenerateRandomData<Key_Size>());
			entry.customers() = { test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>() };
			entry.executors() = { test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>() };
			entry.verifiers() = { test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>(), test::GenerateRandomData<Key_Size>() };
			entry.setStart(Height(12));
			entry.setDuration(BlockDuration(12));
			entry.pushHash(test::GenerateRandomData<Key_Size>(), Height(12));
			entry.pushHash(test::GenerateRandomData<Key_Size>(), Height(13));

			return entry;
		}
	}

	TEST(TEST_CLASS, CanMapContractEntry_ModelToDbModel) {
		// Arrange:
		auto entry = CreateContractEntry();
		auto address = test::GenerateRandomData<Address_Decoded_Size>();

		// Act:
		auto document = ToDbModel(entry, address);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));

		auto contractView = documentView["contract"].get_document().view();
		EXPECT_EQ(9u, test::GetFieldCount(contractView));
		test::AssertEqualContractData(entry, address, contractView);
	}

	// endregion

	// region ToContractEntry

	namespace {
		bsoncxx::document::value CreateDbContractEntry(const Address& address) {
			auto descriptor = CreateContractEntry();
			return ToDbModel(descriptor, address);
		}
	}

	TEST(TEST_CLASS, CanMapContractEntry_DbModelToModel) {
		// Arrange:
		auto address = test::GenerateRandomData<Address_Decoded_Size>();
		auto dbContractEntry = CreateDbContractEntry(address);

		// Act:
		auto entry = ToContractEntry(dbContractEntry);

		// Assert:
		auto view = dbContractEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));

		auto contractView = view["contract"].get_document().view();
		EXPECT_EQ(9u, test::GetFieldCount(contractView));
		test::AssertEqualContractData(entry, address, contractView);
	}

	// endregion
}}}