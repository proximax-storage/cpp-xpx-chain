/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/observers/Observers.h"
#include "tests/test/CommitteeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS AddHarvesterObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(AddHarvester, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::CommitteeCacheFactory>;
		using Notification = model::AddHarvesterNotification<1>;

		const Key Harvester_Key = test::GenerateRandomByteArray<Key>();
		const Key Owner = test::GenerateRandomByteArray<Key>();
		const Height Current_Height = test::GenerateRandomValue<Height>();
		constexpr MosaicId Harvesting_Mosaic_Id(1234);
		constexpr Amount Harvester_Balance(100);

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::CommitteeConfiguration::Uninitialized();
			pluginConfig.InitialActivity = test::Initial_Activity;
			pluginConfig.MinGreed = test::Min_Greed;
			config.Network.SetPluginConfiguration(pluginConfig);
			return config.ToConst();
		}
	}

	TEST(TEST_CLASS, AddHarvester_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfig());
		auto notification = Notification(Owner, Harvester_Key);
		auto pObserver = CreateAddHarvesterObserver();
		auto& committeeCache = context.cache().sub<cache::CommitteeCache>();
		auto expectedCommitteeEntry = test::CreateCommitteeEntry(
			Harvester_Key,
			Owner,
			Height(0),
			Current_Height,
			Importance(Harvester_Balance.unwrap()),
			true,
			test::Initial_Activity,
			test::Min_Greed);

		auto& accountCache = context.cache().sub<cache::AccountStateCache>();
		accountCache.addAccount(Harvester_Key, Current_Height);
		auto& accountState = accountCache.find(Harvester_Key).get();
		accountState.Balances.track(Harvesting_Mosaic_Id);
		accountState.Balances.credit(Harvesting_Mosaic_Id, Harvester_Balance, Current_Height);

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto iter = committeeCache.find(expectedCommitteeEntry.key());
		const auto& actualEntry = iter.get();
		test::AssertEqualCommitteeEntry(expectedCommitteeEntry, actualEntry);
	}

	TEST(TEST_CLASS, AddHarvester_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		auto notification = Notification(Owner, Harvester_Key);
		auto pObserver = CreateAddHarvesterObserver();
		auto& committeeCache = context.cache().sub<cache::CommitteeCache>();
		committeeCache.insert(test::CreateCommitteeEntry(Harvester_Key));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		ASSERT_FALSE(committeeCache.contains(Harvester_Key));
	}
}}
