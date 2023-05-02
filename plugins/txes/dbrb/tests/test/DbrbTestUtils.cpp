/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DbrbTestUtils.h"

namespace catapult { namespace test {

	dbrb::View GenerateRandomView(const size_t& length) {
		dbrb::ViewData viewData;
		for (auto i = 0u; i < length; ++i)
			viewData.emplace(test::GenerateRandomByteArray<dbrb::ProcessId>());

		return dbrb::View{ viewData };
	}

	state::DbrbProcessEntry CreateDbrbProcessEntry(const dbrb::ProcessId& processId, const Timestamp& expirationTime) {
		return state::DbrbProcessEntry(processId, expirationTime);
	}
}}
