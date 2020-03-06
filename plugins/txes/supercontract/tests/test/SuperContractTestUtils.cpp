/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	state::SuperContractEntry CreateSuperContractEntry(
			Key key,
			Height start,
			Height end,
			VmVersion vmVersion,
			Key mainDriveKey,
			Hash256 fileHash) {
		state::SuperContractEntry entry(key);
		entry.setStart(start);
		entry.setEnd(end);
		entry.setVmVersion(vmVersion);
		entry.setMainDriveKey(mainDriveKey);
		entry.setFileHash(fileHash);

		return entry;
	}

	void AssertEqualSuperContractData(const state::SuperContractEntry& entry1, const state::SuperContractEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		EXPECT_EQ(entry1.start(), entry2.start());
		EXPECT_EQ(entry1.end(), entry2.end());
		EXPECT_EQ(entry1.vmVersion(), entry2.vmVersion());
		EXPECT_EQ(entry1.mainDriveKey(), entry2.mainDriveKey());
		EXPECT_EQ(entry1.fileHash(), entry2.fileHash());

	}
}}


