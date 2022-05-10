/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/CommitteeTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS RemoveHarvesterObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(RemoveHarvester, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::CommitteeCacheFactory>;
		using Notification = model::RemoveHarvesterNotification<1>;

		const Key Harvester_Key = test::GenerateRandomByteArray<Key>();
		const Key Owner = test::GenerateRandomByteArray<Key>();
		const Height Current_Height = test::GenerateRandomValue<Height>();
	}

	TEST(TEST_CLASS, RemoveHarvester_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		auto notification = Notification(Owner, Harvester_Key);
		auto pObserver = CreateRemoveHarvesterObserver();
		auto committeeEntry = test::CreateCommitteeEntry(Harvester_Key);
		auto& committeeCache = context.cache().sub<cache::CommitteeCache>();
		committeeCache.insert(committeeEntry);
		auto expectedEntry = committeeEntry;
		expectedEntry.setDisabledHeight(Current_Height);

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto iter = committeeCache.find(committeeEntry.key());
		const auto& actualEntry = iter.get();
		test::AssertEqualCommitteeEntry(expectedEntry, actualEntry);
	}

	TEST(TEST_CLASS, RemoveHarvester_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		auto notification = Notification(Owner, Harvester_Key);
		auto pObserver = CreateRemoveHarvesterObserver();
		auto committeeEntry = test::CreateCommitteeEntry(Harvester_Key);
		committeeEntry.setDisabledHeight(Current_Height);
		auto& committeeCache = context.cache().sub<cache::CommitteeCache>();
		committeeCache.insert(committeeEntry);
		auto expectedEntry = committeeEntry;
		expectedEntry.setDisabledHeight(Height(0));

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		auto iter = committeeCache.find(committeeEntry.key());
		const auto& actualEntry = iter.get();
		test::AssertEqualCommitteeEntry(expectedEntry, actualEntry);
	}
}}
