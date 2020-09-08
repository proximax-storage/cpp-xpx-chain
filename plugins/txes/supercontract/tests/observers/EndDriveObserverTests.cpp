/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/SuperContractTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"

namespace catapult { namespace observers {

#define TEST_CLASS EndDriveObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(EndDrive, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::SuperContractCacheFactory>;
		using Notification = model::EndDriveNotification<1>;

		const auto Current_Height = test::GenerateRandomValue<Height>();
		const auto Drive_Key = test::GenerateRandomByteArray<Key>();

		auto CreateDriveEntry(const std::vector<state::SuperContractEntry>& superContractEntries) {
			state::DriveEntry driveEntry(Drive_Key);
			driveEntry.coowners().insert(test::GenerateRandomByteArray<Key>());
			for (const auto& superContractEntry : superContractEntries)
				driveEntry.coowners().insert(superContractEntry.key());

			return driveEntry;
		}

		auto CreateSuperContractEntry(state::SuperContractState state) {
			state::SuperContractEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setState(state);

			return entry;
		}
	}

	TEST(TEST_CLASS, EndDrive_Commit) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Commit, Current_Height);
		Notification notification(Drive_Key, test::GenerateRandomByteArray<Key>());
		auto pObserver = CreateEndDriveObserver();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
		auto& driveCache = context.cache().sub<cache::DriveCache>();
		std::vector<state::SuperContractEntry> superContractEntries{
			CreateSuperContractEntry(state::SuperContractState::Active),
			CreateSuperContractEntry(state::SuperContractState::DeactivatedByParticipant),
			CreateSuperContractEntry(state::SuperContractState::DeactivatedByDriveEnd),
		};

		// Populate cache.
		for (const auto& entry : superContractEntries)
			superContractCache.insert(entry);
		driveCache.insert(CreateDriveEntry(superContractEntries));
		superContractEntries[0].setState(state::SuperContractState::DeactivatedByDriveEnd);

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		for (const auto& entry : superContractEntries) {
			auto superContractCacheIter = superContractCache.find(entry.key());
			test::AssertEqualSuperContractData(entry, superContractCacheIter.get());
		}
	}

	TEST(TEST_CLASS, EndDrive_Rollback) {
		// Arrange:
		ObserverTestContext context(NotifyMode::Rollback, Current_Height);
		Notification notification(Drive_Key, test::GenerateRandomByteArray<Key>());
		auto pObserver = CreateEndDriveObserver();
		auto& superContractCache = context.cache().sub<cache::SuperContractCache>();
		auto& driveCache = context.cache().sub<cache::DriveCache>();
		std::vector<state::SuperContractEntry> superContractEntries{
			CreateSuperContractEntry(state::SuperContractState::Active),
			CreateSuperContractEntry(state::SuperContractState::DeactivatedByParticipant),
			CreateSuperContractEntry(state::SuperContractState::DeactivatedByDriveEnd),
		};

		// Populate cache.
		for (const auto& entry : superContractEntries)
			superContractCache.insert(entry);
		driveCache.insert(CreateDriveEntry(superContractEntries));
		superContractEntries[2].setState(state::SuperContractState::Active);

		// Act:
		test::ObserveNotification(*pObserver, notification, context);

		// Assert: check the cache
		for (const auto& entry : superContractEntries) {
			auto superContractCacheIter = superContractCache.find(entry.key());
			test::AssertEqualSuperContractData(entry, superContractCacheIter.get());
		}
	}
}}
