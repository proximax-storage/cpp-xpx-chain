/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/committee/src/state/CommitteeEntry.h"
#include <bsoncxx/json.hpp>

namespace catapult { namespace test {

	/// Verifies that \a dbEntry is equivalent to \a entry and \a address.
	void AssertEqualCommitteeData(const state::CommitteeEntry& entry, const Address& address, const bsoncxx::document::view& dbEntry);
}}
