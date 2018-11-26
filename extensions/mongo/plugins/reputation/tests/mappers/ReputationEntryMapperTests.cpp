/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "src/mappers/ReputationEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/test/ReputationMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS ReputationEntryMapperTests

	// region ToDbModel

	namespace {
		state::ReputationEntry CreateReputationEntry() {
			state::ReputationEntry entry(test::GenerateRandomData<Key_Size>());
			entry.setPositiveInteractions(Reputation{12u});
			entry.setNegativeInteractions(Reputation{34u});

			return entry;
		}
	}

	TEST(TEST_CLASS, CanMapReputationEntry_ModelToDbModel) {
		// Arrange:
		auto entry = CreateReputationEntry();
		auto address = test::GenerateRandomData<Address_Decoded_Size>();

		// Act:
		auto document = ToDbModel(entry, address);
		auto documentView = document.view();

		// Assert:
		EXPECT_EQ(1u, test::GetFieldCount(documentView));

		auto reputationView = documentView["reputation"].get_document().view();
		EXPECT_EQ(4u, test::GetFieldCount(reputationView));
		test::AssertEqualReputationData(entry, address, reputationView);
	}

	// endregion

	// region ToReputationEntry

	namespace {
		bsoncxx::document::value CreateDbReputationEntry(const Address& address) {
			auto descriptor = CreateReputationEntry();
			return ToDbModel(descriptor, address);
		}
	}

	TEST(TEST_CLASS, CanMapReputationEntry_DbModelToModel) {
		// Arrange:
		auto address = test::GenerateRandomData<Address_Decoded_Size>();
		auto dbReputationEntry = CreateDbReputationEntry(address);

		// Act:
		auto entry = ToReputationEntry(dbReputationEntry);

		// Assert:
		auto view = dbReputationEntry.view();
		EXPECT_EQ(1u, test::GetFieldCount(view));

		auto reputationView = view["reputation"].get_document().view();
		EXPECT_EQ(4u, test::GetFieldCount(reputationView));
		test::AssertEqualReputationData(entry, address, reputationView);
	}

	// endregion
}}}
