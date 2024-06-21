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
			Key owner,
			const Height& disabledHeight,
			const Height& lastSigningBlockHeight,
			const Importance& effectiveBalance,
			bool canHarvest,
			double activityObsolete,
			double greedObsolete) {
		return { key, owner, lastSigningBlockHeight, effectiveBalance, canHarvest, activityObsolete, greedObsolete, disabledHeight };
	}

	state::AccountData CreateAccountData(
			const Height& lastSigningBlockHeight,
			const Importance& effectiveBalance,
			bool canHarvest,
			double activityObsolete,
			double greedObsolete) {
		return { lastSigningBlockHeight, effectiveBalance, canHarvest, activityObsolete, greedObsolete, Timestamp(0), 0u, 1u, 1u, Key(), BlockchainVersion(0) };
	}

	void AssertEqualAccountData(const state::AccountData& data1, const state::AccountData& data2) {
		EXPECT_EQ(data1.LastSigningBlockHeight, data2.LastSigningBlockHeight);
		EXPECT_EQ(data1.EffectiveBalance, data2.EffectiveBalance);
		EXPECT_EQ(data1.CanHarvest, data2.CanHarvest);
		EXPECT_EQ(data1.ActivityObsolete, data2.ActivityObsolete);
		EXPECT_EQ(data1.GreedObsolete, data2.GreedObsolete);
		EXPECT_EQ(data1.ExpirationTime, data2.ExpirationTime);
		EXPECT_EQ(data1.Activity, data2.Activity);
		EXPECT_EQ(data1.FeeInterest, data2.FeeInterest);
		EXPECT_EQ(data1.FeeInterestDenominator, data2.FeeInterestDenominator);
		EXPECT_EQ(data1.BootKey, data2.BootKey);
	}

	void AssertEqualCommitteeEntry(const state::CommitteeEntry& entry1, const state::CommitteeEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		EXPECT_EQ(entry1.owner(), entry2.owner());
		EXPECT_EQ(entry1.disabledHeight(), entry2.disabledHeight());
		AssertEqualAccountData(entry1.data(), entry2.data());
		EXPECT_EQ(entry1.version(), entry2.version());
	}
}}


