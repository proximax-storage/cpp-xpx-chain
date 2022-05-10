/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteeMapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	void AssertEqualCommitteeData(const state::CommitteeEntry& entry, const Address& address, const bsoncxx::document::view& dbEntry) {
		EXPECT_EQ(9u, test::GetFieldCount(dbEntry));
		EXPECT_EQ(entry.key(), GetKeyValue(dbEntry, "key"));
		EXPECT_EQ(entry.owner(), GetKeyValue(dbEntry, "owner"));
		EXPECT_EQ(address, test::GetAddressValue(dbEntry, "address"));
		EXPECT_EQ(entry.disabledHeight().unwrap(), GetUint64(dbEntry, "disabledHeight"));
		EXPECT_EQ(entry.lastSigningBlockHeight().unwrap(), GetUint64(dbEntry, "lastSigningBlockHeight"));
		EXPECT_EQ(entry.effectiveBalance().unwrap(), GetUint64(dbEntry, "effectiveBalance"));
		EXPECT_EQ(entry.canHarvest(), GetBool(dbEntry, "canHarvest"));
		EXPECT_EQ(entry.activity(), GetDouble(dbEntry, "activity"));
		EXPECT_EQ(entry.greed(), GetDouble(dbEntry, "greed"));
	}
}}
