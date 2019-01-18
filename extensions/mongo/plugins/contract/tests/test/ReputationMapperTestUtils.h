/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/contract/src/state/ReputationEntry.h"
#include <bsoncxx/json.hpp>

namespace catapult { namespace test {

	/// Verifies that db reputation (\a dbReputation) is equivalent to model reputation \a entry and \a address.
	void AssertEqualReputationData(const state::ReputationEntry& entry, const Address& address, const bsoncxx::document::view& dbReputation);
}}
