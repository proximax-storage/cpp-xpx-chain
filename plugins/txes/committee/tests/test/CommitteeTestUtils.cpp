/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommitteeTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	state::CommitteeEntry CreateCommitteeEntry(
			Key key,
			const Height& lastSigningBlockHeight,
			const Importance& effectiveBalance,
			bool canHarvest,
			double activity,
			double greed) {
		return state::CommitteeEntry(key, lastSigningBlockHeight, effectiveBalance, canHarvest, activity, greed);
	}

	state::AccountData CreateAccountData(
			const Height& lastSigningBlockHeight,
			const Importance& effectiveBalance,
			bool canHarvest,
			double activity,
			double greed) {
		return state::AccountData(lastSigningBlockHeight, effectiveBalance, canHarvest, activity, greed);
	}

	void AssertEqualAccountData(const state::AccountData& data1, const state::AccountData& data2) {
		EXPECT_EQ(data1.LastSigningBlockHeight, data2.LastSigningBlockHeight);
		EXPECT_EQ(data1.EffectiveBalance, data2.EffectiveBalance);
		EXPECT_EQ(data1.Greed, data2.Greed);
		EXPECT_EQ(data1.CanHarvest, data2.CanHarvest);
		EXPECT_EQ(data1.Activity, data2.Activity);
	}

	void AssertEqualCommitteeEntry(const state::CommitteeEntry& entry1, const state::CommitteeEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		AssertEqualAccountData(entry1.data(), entry2.data());
	}
}}


