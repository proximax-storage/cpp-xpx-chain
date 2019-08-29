/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/config/src/state/NetworkConfigEntry.h"
#include <bsoncxx/json.hpp>

namespace catapult { namespace test {

	/// Verifies that \a dbConfigEntry is equivalent to \a entry.
	void AssertEqualNetworkConfigData(const state::NetworkConfigEntry& entry, const bsoncxx::document::view& dbConfigEntry);
}}
