/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/service/src/state/DriveEntry.h"
#include <bsoncxx/json.hpp>

namespace catapult { namespace test {

	/// Verifies that db drive entry (\a dbDriveEntry) is equivalent to model drive \a entry and \a address.
	void AssertEqualDriveData(const state::DriveEntry& entry, const Address& address, const bsoncxx::document::view& dbDriveEntry);
}}
