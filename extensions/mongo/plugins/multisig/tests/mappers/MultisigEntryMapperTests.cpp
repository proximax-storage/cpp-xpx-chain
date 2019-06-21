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

#include "src/mappers/MultisigEntryMapper.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/test/MultisigMapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace mongo { namespace plugins {

#define TEST_CLASS MultisigEntryMapperTests

	// region ToDbModel

	namespace {
		void InsertRandom(utils::SortedKeySet& keys, size_t count) {
			for (auto i = 0u; i < count; ++i)
				keys.insert(test::GenerateRandomByteArray<Key>());
		}

		state::MultisigEntry CreateMultisigEntry(uint8_t numCosignatories, uint8_t numMultisigAccounts) {
			state::MultisigEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setMinApproval(12);
			entry.setMinRemoval(23);

			InsertRandom(entry.cosignatories(), numCosignatories);
			InsertRandom(entry.multisigAccounts(), numMultisigAccounts);

			return entry;
		}

		void AssertCanMapMultisigEntry(uint8_t numCosignatories, uint8_t numMultisigAccounts) {
			// Arrange:
			auto entry = CreateMultisigEntry(numCosignatories, numMultisigAccounts);
			auto address = test::GenerateRandomByteArray<Address>();

			// Act:
			auto document = ToDbModel(entry, address);
			auto documentView = document.view();

			// Assert:
			EXPECT_EQ(1u, test::GetFieldCount(documentView));

			auto multisigView = documentView["multisig"].get_document().view();
			EXPECT_EQ(6u, test::GetFieldCount(multisigView));
			test::AssertEqualMultisigData(entry, address, multisigView);
		}
	}

	TEST(TEST_CLASS, CanMapMultisigEntryWithNeitherCosignatoriesNorMultisigAccounts_ModelToDbModel) {
		// Assert:
		AssertCanMapMultisigEntry(0, 0);
	}

	TEST(TEST_CLASS, CanMapMultisigEntryWithCosignatoriesButNoMultisigAccounts_ModelToDbModel) {
		// Assert:
		AssertCanMapMultisigEntry(5, 0);
	}

	TEST(TEST_CLASS, CanMapMultisigEntryWithoutCosignatoriesButWithMultisigAccounts_ModelToDbModel) {
		// Assert:
		AssertCanMapMultisigEntry(0, 5);
	}

	TEST(TEST_CLASS, CanMapMultisigEntryWithCosignatoriesAndWithMultisigAccounts_ModelToDbModel) {
		// Assert:
		AssertCanMapMultisigEntry(4, 5);
	}

	// endregion

	// region ToMultisigEntry

	namespace {
		bsoncxx::document::value CreateDbMultisigEntry(const Address& address, uint8_t numCosignatories, uint8_t numMultisigAccounts) {
			return ToDbModel(CreateMultisigEntry(numCosignatories, numMultisigAccounts), address);
		}

		void AssertCanMapDbMultisigEntry(uint8_t numCosignatories, uint8_t numMultisigAccounts) {
			// Arrange:
			auto address = test::GenerateRandomByteArray<Address>();
			auto dbMultisigEntry = CreateDbMultisigEntry(address, numCosignatories, numMultisigAccounts);

			// Act:
			auto entry = ToMultisigEntry(dbMultisigEntry);

			// Assert:
			auto view = dbMultisigEntry.view();
			EXPECT_EQ(1u, test::GetFieldCount(view));

			auto multisigView = view["multisig"].get_document().view();
			EXPECT_EQ(6u, test::GetFieldCount(multisigView));
			test::AssertEqualMultisigData(entry, address, multisigView);
		}
	}

	TEST(TEST_CLASS, CanMapMultisigEntryWithNeitherCosignatoriesNorMultisigAccounts_DbModelToModel) {
		// Assert:
		AssertCanMapDbMultisigEntry(0, 0);
	}

	TEST(TEST_CLASS, CanMapMultisigEntryWithCosignatoriesButNoMultisigAccounts_DbModelToModel) {
		// Assert:
		AssertCanMapDbMultisigEntry(5, 0);
	}

	TEST(TEST_CLASS, CanMapMultisigEntryWithoutCosignatoriesButWithMultisigAccounts_DbModelToModel) {
		// Assert:
		AssertCanMapDbMultisigEntry(0, 5);
	}

	TEST(TEST_CLASS, CanMapMultisigEntryWithCosignatoriesAndWithMultisigAccounts_DbModelToModel) {
		// Assert:
		AssertCanMapDbMultisigEntry(4, 5);
	}

	// endregion
}}}
