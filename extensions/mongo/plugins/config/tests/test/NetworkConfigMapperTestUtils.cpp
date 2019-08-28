/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "NetworkConfigMapperTestUtils.h"
#include "mongo/src/mappers/MapperUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	void AssertEqualNetworkConfigData(const state::NetworkConfigEntry& entry, const bsoncxx::document::view& dbConfigEntry) {
		EXPECT_EQ(entry.height(), mongo::mappers::GetValue64<Height>(dbConfigEntry["height"]));
		EXPECT_EQ(entry.networkConfig(), dbConfigEntry["networkConfig"].get_utf8().value.to_string());
		EXPECT_EQ(entry.supportedEntityVersions(), dbConfigEntry["supportedEntityVersions"].get_utf8().value.to_string());
	}
}}
