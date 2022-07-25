/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"
#include "tests/test/cache/BalanceTransferTestUtils.h"
#include "catapult/model/Address.h"

namespace catapult { namespace observers {

#define TEST_CLASS ReplicatorOffboardingObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(ReplicatorOffboarding,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::ReplicatorCacheFactory>;
        using Notification = model::ReplicatorOffboardingNotification<1>;

		Key Replicator_Key = test::GenerateRandomByteArray<Key>();
		Key Drive_Key1 = test::GenerateRandomByteArray<Key>();
		Key Drive_Key2 = test::GenerateRandomByteArray<Key>();
		Key Target_Drive_Key = Drive_Key2;	// Drive key that is mentioned in notification
        constexpr auto Current_Height = Height(25);
		constexpr auto Capacity = Amount(30);
		constexpr auto Min_Replicator_Count = 4;
		constexpr uint64_t Drive1_Size = 100;
		constexpr uint64_t Drive1_Used_Size = 40;
		constexpr uint16_t Drive1_Replicator_Count = 6;
		constexpr uint16_t Drive1_Actual_Replicator_Count = 5;
		constexpr uint64_t Drive2_Size = 200;
		constexpr uint64_t Drive2_Confirmed_Used_Size = 50;
		constexpr uint64_t Drive2_Used_Size = 60;
		constexpr uint16_t Drive2_Replicator_Count = 4;
		constexpr uint16_t Drive2_Actual_Replicator_Count = 4;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::StorageConfiguration::Uninitialized();
			pluginConfig.MinReplicatorCount = Min_Replicator_Count;
			config.Network.SetPluginConfiguration(pluginConfig);
			return config.ToConst();
		}

        std::vector<state::BcDriveEntry> CreateInitialDriveEntries() {
			std::vector<state::BcDriveEntry> ret;
            state::BcDriveEntry entry1(Drive_Key1);
            entry1.setSize(Drive1_Size);
			entry1.setUsedSizeBytes(Drive1_Used_Size);
			entry1.setReplicatorCount(Drive1_Replicator_Count);
			entry1.replicators().emplace(Replicator_Key);
			for (auto i = 0u; i < Drive1_Actual_Replicator_Count - 1; ++i)
				entry1.replicators().emplace(test::GenerateRandomByteArray<Key>());
			ret.push_back(entry1);

            state::BcDriveEntry entry2(Drive_Key2);
            entry2.setSize(Drive2_Size);
			entry2.setUsedSizeBytes(Drive2_Used_Size);
			entry2.setReplicatorCount(Drive2_Replicator_Count);
			entry2.replicators().emplace(Replicator_Key);
			for (auto i = 0u; i < Drive2_Actual_Replicator_Count - 1; ++i)
				entry2.replicators().emplace(test::GenerateRandomByteArray<Key>());
            entry2.confirmedUsedSizes().insert({ Replicator_Key, Drive2_Confirmed_Used_Size });
			ret.push_back(entry2);

            return ret;
        }

		std::vector<state::BcDriveEntry> CreateExpectedDriveEntries(std::vector<state::BcDriveEntry> initialDriveEntries) {
			for (auto& entry : initialDriveEntries)
				if (entry.key() == Target_Drive_Key) {
					entry.offboardingReplicators().emplace_back(Replicator_Key);
					break;
				}

			return initialDriveEntries;
		}

		state::PriorityQueueEntry CreateDriveQueueEntry(const std::vector<state::BcDriveEntry>& driveEntries) {
			state::PriorityQueueEntry driveQueueEntry(state::DrivePriorityQueueKey);
			for (const auto& driveEntry : driveEntries) {
				if (driveEntry.replicators().size() < driveEntry.replicatorCount())
					driveQueueEntry.set(driveEntry.key(), utils::CalculateDrivePriority(driveEntry, Min_Replicator_Count));
			}

			return driveQueueEntry;
		}

        state::ReplicatorEntry CreateReplicatorEntry() {
            state::ReplicatorEntry entry(Replicator_Key);
            entry.drives().emplace(Drive_Key1, state::DriveInfo{});
            entry.drives().emplace(Drive_Key2, state::DriveInfo{});
            return entry;
        }

        void RunTest(NotifyMode mode) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height, CreateConfig());
            Notification notification(Replicator_Key, Target_Drive_Key);
            auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
            auto& driveCache = context.cache().sub<cache::BcDriveCache>();
			auto& priorityQueueCache = context.cache().sub<cache::PriorityQueueCache>();

            //Populate cache
            replicatorCache.insert(CreateReplicatorEntry());
			auto initialDriveEntries = CreateInitialDriveEntries();
			for (const auto& driveEntry : initialDriveEntries)
            	driveCache.insert(driveEntry);
			auto initialDriveQueueEntry = CreateDriveQueueEntry(initialDriveEntries);
			priorityQueueCache.insert(initialDriveQueueEntry);

			auto pObserver = CreateReplicatorOffboardingObserver();

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			auto expectedDriveEntries = CreateExpectedDriveEntries(initialDriveEntries);
			for (const auto& expectedEntry : expectedDriveEntries) {
				auto driveIter = driveCache.find(expectedEntry.key());
				auto& driveEntry = driveIter.get();
				test::AssertEqualBcDriveData(expectedEntry, driveEntry);
			}

			auto expectedDriveQueue = CreateDriveQueueEntry(expectedDriveEntries).priorityQueue();
			auto& driveQueue = getPriorityQueueEntry(priorityQueueCache, state::DrivePriorityQueueKey).priorityQueue();
			EXPECT_EQ(driveQueue.size(), expectedDriveQueue.size());
			while (!driveQueue.empty()) {
				EXPECT_EQ(driveQueue.top(), expectedDriveQueue.top());
				driveQueue.pop();
				expectedDriveQueue.pop();
			}
        }
    }

    TEST(TEST_CLASS, ReplicatorOffboarding_Commit) {
        // Assert:
        RunTest(NotifyMode::Commit);
    }

    TEST(TEST_CLASS, DriveClosure_Rollback) {
        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback), catapult_runtime_error);
    }
}}