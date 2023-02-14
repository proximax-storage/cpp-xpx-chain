/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractTestUtils.h"

namespace catapult { namespace test {

	state::SuperContractEntry CreateSuperContractEntry(Key key) {
		state::SuperContractEntry entry(key);
		return entry;
	}
	state::DriveContractEntry CreateDriveContractEntry(Key key) {
		state::DriveContractEntry entry(key);
		return entry;
	}

}}