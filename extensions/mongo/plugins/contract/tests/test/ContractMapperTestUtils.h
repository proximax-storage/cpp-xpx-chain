/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/contract/src/state/ContractEntry.h"
#include <bsoncxx/json.hpp>

namespace catapult { namespace test {

	/// Verifies that db contract (\a dbContract) is equivalent to model contract \a entry and \a address.
	void AssertEqualContractData(const state::ContractEntry& entry, const Address& address, const bsoncxx::document::view& dbContract);
}}
