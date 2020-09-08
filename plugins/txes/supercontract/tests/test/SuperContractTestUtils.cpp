/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractTestUtils.h"

namespace catapult { namespace test {

	state::SuperContractEntry CreateSuperContractEntry(
			Key key,
			state::SuperContractState state,
			Key owner,
			Height start,
			Height end,
			VmVersion vmVersion,
			Key mainDriveKey,
			Hash256 fileHash,
			uint16_t executionCount) {
		state::SuperContractEntry entry(key);
		entry.setState(state);
		entry.setOwner(owner);
		entry.setStart(start);
		entry.setEnd(end);
		entry.setVmVersion(vmVersion);
		entry.setMainDriveKey(mainDriveKey);
		entry.setFileHash(fileHash);
		entry.setExecutionCount(executionCount);

		return entry;
	}

	void AssertEqualSuperContractData(const state::SuperContractEntry& entry1, const state::SuperContractEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		EXPECT_EQ(entry1.state(), entry2.state());
		EXPECT_EQ(entry1.owner(), entry2.owner());
		EXPECT_EQ(entry1.start(), entry2.start());
		EXPECT_EQ(entry1.end(), entry2.end());
		EXPECT_EQ(entry1.vmVersion(), entry2.vmVersion());
		EXPECT_EQ(entry1.mainDriveKey(), entry2.mainDriveKey());
		EXPECT_EQ(entry1.fileHash(), entry2.fileHash());
		EXPECT_EQ(entry1.executionCount(), entry2.executionCount());

	}
}}


