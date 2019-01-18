/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/


#include "ReputationMapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "mongo/tests/test/MapperTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	void AssertEqualReputationData(const state::ReputationEntry& entry, const Address& address, const bsoncxx::document::view& dbReputation) {
		EXPECT_EQ(address, test::GetAddressValue(dbReputation, "accountAddress"));

		EXPECT_EQ(entry.key(), GetKeyValue(dbReputation, "account"));
		EXPECT_EQ(entry.positiveInteractions().unwrap(), GetUint64(dbReputation, "positiveInteractions"));
		EXPECT_EQ(entry.negativeInteractions().unwrap(), GetUint64(dbReputation, "negativeInteractions"));
	}
}}
