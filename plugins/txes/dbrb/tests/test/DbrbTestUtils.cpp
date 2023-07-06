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

	void AssertEqualDbrbProcessEntry(const state::DbrbProcessEntry& entry1, const state::DbrbProcessEntry& entry2) {
		//EXPECT_EQ(entry1.processId(), entry2.processId());
		EXPECT_EQ_MEMORY(entry1.processId().data(), entry2.processId().data(), Key_Size);
		EXPECT_EQ(entry1.expirationTime(), entry2.expirationTime());
		EXPECT_EQ(entry1.version(), entry2.version());
	}

}}
