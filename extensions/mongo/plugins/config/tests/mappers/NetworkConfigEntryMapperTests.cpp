/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "src/mappers/NetworkConfigEntryMapper.h"
#include "tests/test/NetworkConfigMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS NetworkConfigEntryMapperTests

	// region ToDbModel

	namespace {
		state::NetworkConfigEntry CreateNetworkConfigEntry() {
			return state::NetworkConfigEntry(Height(), "aaa", "bbb");
		}
	}

	TEST(TEST_CLASS, CanMapNetworkConfigEntry_ModelToDbModel) {
		// Arrange:
		auto entry = CreateNetworkConfigEntry();

		// Act:
		auto document = ToDbModel(entry);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));

		auto networkConfigView = documentView["networkConfig"].get_document().view();
		EXPECT_EQ(3u, test::GetFieldCount(networkConfigView));
		test::AssertEqualNetworkConfigData(entry, networkConfigView);
	}

	// endregion

	// region ToNetworkConfigEntry

	namespace {
		bsoncxx::document::value CreateDbNetworkConfigEntry() {
			return ToDbModel(CreateNetworkConfigEntry());
		}
	}

	TEST(TEST_CLASS, CanMapNetworkConfigEntry_DbModelToModel) {
		// Arrange:
		auto dbNetworkConfigEntry = CreateDbNetworkConfigEntry();

		// Act:
		auto entry = ToNetworkConfigEntry(dbNetworkConfigEntry);

		// Assert:
		auto view = dbNetworkConfigEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));

		auto networkConfigView = view["networkConfig"].get_document().view();
		EXPECT_EQ(3u, test::GetFieldCount(networkConfigView));
		test::AssertEqualNetworkConfigData(entry, networkConfigView);
	}

	// endregion
}}}