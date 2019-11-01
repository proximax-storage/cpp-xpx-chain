/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "plugins/txes/exchange/src/state/ExchangeEntry.h"
#include <bsoncxx/json.hpp>

namespace catapult { namespace test {

	/// Verifies that db offer entry (\a dbExchangeEntry) is equivalent to model offer \a entry.
	void AssertEqualExchangeData(const state::ExchangeEntry& entry, const bsoncxx::document::view& dbExchangeEntry);
}}
