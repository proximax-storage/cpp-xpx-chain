/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/core/NotificationTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS DeactivateObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(Deactivate, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::DeactivateNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto Super_Contract_Key = test::GenerateRandomByteArray<Key>();

		auto CreateSuperContractEntry(state::SuperContractState state) {
			state::SuperContractEntry entry(Super_Contract_Key);
			entry.setState(state);

			return entry;
		}
	}

	TEST(TEST_CLASS, Deactivate_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		Notification notification(test::GenerateRandomByteArray<Key>(), Super_Contract_Key, test::GenerateRandomByteArray<Key>());
		auto pObserver = CreateDeactivateObserver();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

		// Populate cache.
		superContractCache.insert(CreateSuperContractEntry(state::SuperContractState::Active));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		test::AssertEqualSuperContractData(CreateSuperContractEntry(state::SuperContractState::DeactivatedByParticipant), superContractCacheIter.get());
	}

	TEST(TEST_CLASS, Deactivate_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		Notification notification(test::GenerateRandomByteArray<Key>(), Super_Contract_Key, test::GenerateRandomByteArray<Key>());
		auto pObserver = CreateDeactivateObserver();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();

		// Populate cache.
		superContractCache.insert(CreateSuperContractEntry(state::SuperContractState::DeactivatedByParticipant));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto superContractCacheIter = superContractCache.find(Super_Contract_Key);
		test::AssertEqualSuperContractData(CreateSuperContractEntry(state::SuperContractState::Active), superContractCacheIter.get());
	}
}}
