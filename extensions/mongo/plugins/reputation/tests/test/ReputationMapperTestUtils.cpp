/**
*** Copyright (c) 2018-present,
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
