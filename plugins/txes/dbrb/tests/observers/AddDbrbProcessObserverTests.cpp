/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/cache_core/AccountStateCache.h"
#include "src/observers/Observers.h"
#include "tests/test/DbrbTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS DbrbProcessObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(AddDbrbProcess, )

	namespace {
		
//		using ObserverTestContext = test::ObserverTestContextT<test::DbrbProcessCacheFactory>;
//		using Notification = model::AddDbrbProcessNotification<1>;
//
//		const Key Harvester_Key = test::GenerateRandomByteArray<Key>();
//		//Timestamp Expiration_Time = Timestamp(0);
//		const Key Owner = test::GenerateRandomByteArray<Key>();
//		const Height Current_Height = test::GenerateRandomValue<Height>();
////		constexpr MosaicId Harvesting_Mosaic_Id(1234);
////		constexpr Amount Harvester_Balance(100);
//
//		auto CreateConfig() {
//			test::MutableBlockchainConfiguration config;
//			auto pluginConfig = config::DbrbConfiguration::Uninitialized();
//			config.Network.SetPluginConfiguration(pluginConfig);
//			return config.ToConst();
//		}
	}

//	TEST(TEST_CLASS, AddDbrbProcess_Commit) {
//		// Arrange:
//		ObserverTestContext context(NotifyMode::Commit, Current_Height, CreateConfig());
//		auto notification = Notification(Owner);
//		auto pObserver = CreateAddDbrbProcessObserver();
//		auto& dbrbProcessCache = context.cache().sub<cache::DbrbViewCache>();
//		auto expectedDbrbProcessEntry = test::CreateDbrbProcessEntry(Owner);
//
////		auto& accountCache = context.cache().sub<cache::AccountStateCache>();
////		accountCache.addAccount(Harvester_Key, Current_Height);
////		auto& accountState = accountCache.find(Harvester_Key).get();
////		accountState.Balances.track(Harvesting_Mosaic_Id);
////		accountState.Balances.credit(Harvesting_Mosaic_Id, Harvester_Balance, Current_Height);
//
//		// Act:
//		test::ObserveNotification(*pObserver, notification, context);
//
//		// Assert: check the cache
//		auto iter = dbrbProcessCache.find(notification.ProcessId);
//		const auto& actualEntry = iter.get();
//		test::AssertEqualDbrbProcessEntry(expectedDbrbProcessEntry, actualEntry);
//	}

//	TEST(TEST_CLASS, AddDbrbProcess_Rollback) {
//		// Arrange:
//		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
//		auto notification = Notification(Owner);
//		auto pObserver = CreateAddDbrbProcessObserver();
//		auto& dbrbProcessCache = context.cache().sub<cache::DbrbViewCache>();
//		dbrbProcessCache.insert(test::CreateDbrbProcessEntry(Harvester_Key));
//
//		// Act:
//		test::ObserveNotification(*pObserver, notification, context);
//
//		// Assert: check the cache
//		ASSERT_FALSE(dbrbProcessCache.contains(Harvester_Key));
//	}
}}
