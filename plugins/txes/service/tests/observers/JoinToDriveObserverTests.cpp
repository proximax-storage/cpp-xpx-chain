/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/ServiceTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS JoinToDriveObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(JoinToDrive, )

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::DriveCacheFactory>;
		using Notification = model::JoinToDriveNotification<1>;

		constexpr Height Current_Height(10);
		const Key Replicator_Key = test::GenerateRandomByteArray<Key>();
		constexpr uint16_t Replicas(100);
		constexpr uint8_t Percent_Approvers(60);

		void AddFiles(state::DriveEntry& entry, uint8_t numFiles) {
			for (auto i = 0u; i < numFiles; ++i) {
				auto fileHash = test::GenerateRandomByteArray<Hash256>();
				uint64_t fileSize = (i + 1) * 100;
				entry.files().emplace(fileHash, state::FileInfo{ fileSize });
			}
		}

		state::DriveEntry CreateInitialDriveEntry(
				uint8_t numFiles,
				uint8_t numReplicators,
				uint8_t minReplicators,
				uint8_t numBillingPeriods,
				state::DriveState state) {
			state::DriveEntry entry(test::GenerateRandomByteArray<Key>());
			entry.setState(state);
			entry.setPercentApprovers(Percent_Approvers);
			entry.setReplicas(Replicas);
			entry.setMinReplicators(minReplicators);

			for (auto i = 0u; i < numReplicators; ++i) {
				entry.replicators().emplace(test::GenerateRandomByteArray<Key>(), state::ReplicatorInfo());
			}

			for (auto i = 0u; i < numBillingPeriods; ++i) {
				entry.billingHistory().push_back(state::BillingPeriodDescription{Height(), Height(), {}});
			}

			AddFiles(entry, numFiles);

			return entry;
		}

		state::DriveEntry CreateExpectedDriveEntry(const state::DriveEntry& initialEntry, state::DriveState state) {
			state::DriveEntry entry(initialEntry);
			entry.setState(state);
			state::ReplicatorInfo replicator;
			replicator.Start = Current_Height;
			for (const auto& file : entry.files())
				replicator.ActiveFilesWithoutDeposit.insert(file.first);
			entry.replicators().emplace(Replicator_Key, replicator);

			return entry;
		}

		state::MultisigEntry CreateMultisigEntry(const Key& key, uint8_t minApproval, uint8_t minRemoval) {
			state::MultisigEntry entry(key);
			entry.setMinApproval(minApproval);
			entry.setMinRemoval(minRemoval);

			return entry;
		}

		struct CacheValues {
			std::vector<state::DriveEntry> InitialDriveEntries;
			std::vector<state::DriveEntry> ExpectedDriveEntries;
			std::vector<state::MultisigEntry> InitialMultisigEntries;
			std::vector<state::MultisigEntry> ExpectedMultisigEntries;
		};

		void PrepareEntries(CacheValues& values) {
			auto initialEntry = CreateInitialDriveEntry(10, 5, 7, 0, state::DriveState::NotStarted);
			values.InitialDriveEntries.push_back(initialEntry);
			values.ExpectedDriveEntries.push_back(CreateExpectedDriveEntry(initialEntry, state::DriveState::NotStarted));
			values.InitialMultisigEntries.push_back(CreateMultisigEntry(initialEntry.key(), 3, 3));
			values.ExpectedMultisigEntries.push_back(CreateMultisigEntry(initialEntry.key(), 4, 4));

			initialEntry = CreateInitialDriveEntry(10, 6, 7, 0, state::DriveState::NotStarted);
			values.InitialDriveEntries.push_back(initialEntry);
			values.ExpectedDriveEntries.push_back(CreateExpectedDriveEntry(initialEntry, state::DriveState::Pending));
			values.InitialMultisigEntries.push_back(CreateMultisigEntry(initialEntry.key(), 4, 4));
			values.ExpectedMultisigEntries.push_back(CreateMultisigEntry(initialEntry.key(), 5, 5));

			initialEntry = CreateInitialDriveEntry(10, 7, 7, 1, state::DriveState::InProgress);
			values.InitialDriveEntries.push_back(initialEntry);
			values.ExpectedDriveEntries.push_back(CreateExpectedDriveEntry(initialEntry, state::DriveState::InProgress));
			values.InitialMultisigEntries.push_back(CreateMultisigEntry(initialEntry.key(), 5, 5));
			values.ExpectedMultisigEntries.push_back(CreateMultisigEntry(initialEntry.key(), 5, 5));

			initialEntry = CreateInitialDriveEntry(10, 8, 7, 1, state::DriveState::Finished);
			values.InitialDriveEntries.push_back(initialEntry);
			values.ExpectedDriveEntries.push_back(CreateExpectedDriveEntry(initialEntry, state::DriveState::Finished));
			values.InitialMultisigEntries.push_back(CreateMultisigEntry(initialEntry.key(), 5, 5));
			values.ExpectedMultisigEntries.push_back(CreateMultisigEntry(initialEntry.key(), 6, 6));
		}

		void RunTest(
				NotifyMode mode,
				const state::DriveEntry& initialDriveEntry,
				const state::DriveEntry& expectedDriveEntry,
				const state::MultisigEntry& initialMultisigEntry,
				const state::MultisigEntry& expectedMultisigEntry) {
			// Arrange:
			ObserverTestContext context(mode, Current_Height);
			Notification notification(initialDriveEntry.key(), Replicator_Key);
			auto pObserver = CreateJoinToDriveObserver();
			auto& driveCache = context.cache().sub<cache::DriveCache>();
			auto& multisigCache = context.cache().sub<cache::MultisigCache>();

			// Populate drive cache.
			driveCache.insert(initialDriveEntry);
			multisigCache.insert(initialMultisigEntry);

			// Act:
			test::ObserveNotification(*pObserver, notification, context);

			// Assert: check the cache
			auto driveIter = driveCache.find(expectedDriveEntry.key());
			const auto& actualDriveEntry = driveIter.get();
			test::AssertEqualDriveData(expectedDriveEntry, actualDriveEntry);

			auto multisigIter = multisigCache.find(expectedMultisigEntry.key());
			const auto& actualMultisigEntry = multisigIter.get();
			EXPECT_EQ(expectedMultisigEntry.minApproval(), actualMultisigEntry.minApproval());
			EXPECT_EQ(expectedMultisigEntry.minRemoval(), actualMultisigEntry.minRemoval());
		}
	}

	TEST(TEST_CLASS, JoinToDrive_Commit) {
		// Arrange:
		CacheValues values;
		PrepareEntries(values);

		// Assert:
		for (auto i = 0u; i < values.InitialDriveEntries.size(); ++i) {
			RunTest(
				NotifyMode::Commit,
				values.InitialDriveEntries[i],
				values.ExpectedDriveEntries[i],
				values.InitialMultisigEntries[i],
				values.ExpectedMultisigEntries[i]);
		}
	}

	TEST(TEST_CLASS, JoinToDrive_Rollback) {
		// Arrange:
		CacheValues values;
		PrepareEntries(values);

		// Assert:
		for (auto i = 0u; i < values.InitialDriveEntries.size(); ++i) {
			RunTest(
				NotifyMode::Rollback,
				values.ExpectedDriveEntries[i],
				values.InitialDriveEntries[i],
				values.ExpectedMultisigEntries[i],
				values.InitialMultisigEntries[i]);
		}
	}
}}
