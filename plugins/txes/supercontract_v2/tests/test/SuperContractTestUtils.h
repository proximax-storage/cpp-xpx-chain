/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/state/SuperContractEntry.h"
#include "src/state/DriveContractEntry.h"
#include "tests/test/nodeps/Random.h"

namespace catapult { namespace test {

	state::SuperContractEntry CreateSuperContractEntry(Key key = test::GenerateRandomByteArray<Key>());

	state::DriveContractEntry CreateDriveContractEntry(Key key = test::GenerateRandomByteArray<Key>());
}}