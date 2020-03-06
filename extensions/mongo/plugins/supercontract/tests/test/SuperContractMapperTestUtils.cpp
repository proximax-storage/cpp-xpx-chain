/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractMapperTestUtils.h"
#include "plugins/txes/supercontract/src/state/SuperContractEntry.h"
#include "tests/TestHarness.h"

using namespace catapult::mongo::mappers;

namespace catapult { namespace test {

	void AssertEqualMongoSuperContractData(
			const state::SuperContractEntry& entry,
			const Address& address,
			const bsoncxx::document::view& dbSuperContractEntry) {
		EXPECT_EQ(entry.key(), GetKeyValue(dbSuperContractEntry, "multisig"));
		EXPECT_EQ(address, GetAddressValue(dbSuperContractEntry, "multisigAddress"));
		EXPECT_EQ(entry.start().unwrap(), GetUint64(dbSuperContractEntry, "start"));
		EXPECT_EQ(entry.end().unwrap(), GetUint64(dbSuperContractEntry, "end"));
		EXPECT_EQ(entry.mainDriveKey(), GetKeyValue(dbSuperContractEntry, "mainDriveKey"));
		EXPECT_EQ(entry.fileHash(), GetHashValue(dbSuperContractEntry, "fileHash"));
		EXPECT_EQ(entry.vmVersion().unwrap(), GetUint64(dbSuperContractEntry, "vmVersion"));
	}
}}
