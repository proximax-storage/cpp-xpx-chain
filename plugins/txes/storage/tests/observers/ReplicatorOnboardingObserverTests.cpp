/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
#include "catapult/model/StorageNotifications.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS ReplicatorOnboardingObserverTests

	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

    DEFINE_COMMON_OBSERVER_TESTS(ReplicatorOnboarding, std::make_unique<DriveQueue>())

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::StorageCacheFactory>;
        using Notification = model::ReplicatorOnboardingNotification<1>;

        const Key Replicator_Key = test::GenerateRandomByteArray<Key>();
        constexpr auto Capacity = Amount(50);
		constexpr auto Min_Replicator_Count = 4;
		const Key Drive1_Key = test::GenerateRandomByteArray<Key>();	// Requires a replicator, has the highest priority
		constexpr uint64_t Drive1_Size = 30;
		constexpr uint16_t Drive1_Replicator_Count = 4;
		constexpr uint16_t Drive1_Actual_Replicator_Count = 3;
		const Key Drive2_Key = test::GenerateRandomByteArray<Key>();	// Requires a replicator, but has too large size
		constexpr uint64_t Drive2_Size = 100;
		constexpr uint16_t Drive2_Replicator_Count = 5;
		constexpr uint16_t Drive2_Actual_Replicator_Count = 4;
		const Key Drive3_Key = test::GenerateRandomByteArray<Key>();	// Requires a replicator, but has the lowest priority,
		constexpr uint64_t Drive3_Size = 25;							// and replicator runs out of available capacity at this point
		constexpr uint16_t Drive3_Replicator_Count = 6;
		constexpr uint16_t Drive3_Actual_Replicator_Count = 5;
        constexpr auto Current_Height = Height(25);
		constexpr auto Storage_Mosaic_Id = MosaicId(1234);
		constexpr auto Streaming_Mosaic_Id = MosaicId(4321);

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.StorageMosaicId = Storage_Mosaic_Id;
			config.Immutable.StreamingMosaicId = Streaming_Mosaic_Id;

			auto pluginConfig = config::StorageConfiguration::Uninitialized();
			pluginConfig.MinReplicatorCount = Min_Replicator_Count;
			config.Network.SetPluginConfiguration(pluginConfig);

			return config.ToConst();
		}

		struct TestValues {
		public:
			explicit TestValues()
					: InitialDriveEntries()
					, ExpectedDriveEntries()
					, InitialDriveQueue()
					, ExpectedDriveQueue()
					, ExpectedReplicatorEntry(Replicator_Key)
			{}

		public:
			std::vector<state::BcDriveEntry> InitialDriveEntries;
			std::vector<state::BcDriveEntry> ExpectedDriveEntries;
			DriveQueue InitialDriveQueue;
			DriveQueue ExpectedDriveQueue;
			state::ReplicatorEntry ExpectedReplicatorEntry;
		};

		void AddDriveToTestValues(TestValues& values,
	  			const Key& key, const uint64_t& size, const uint16_t& replicatorCount,
			  	const uint16_t& actualReplicatorCount, const bool& expectingAssignment) {
			auto& initialEntry = values.InitialDriveEntries.emplace_back(key);
			initialEntry.setSize(size);
			initialEntry.setReplicatorCount(replicatorCount);
			for (auto i = 0u; i < actualReplicatorCount; ++i)
				initialEntry.replicators().emplace(test::GenerateRandomByteArray<Key>());
			values.ExpectedDriveEntries.push_back(initialEntry);
			auto& expectedEntry = values.ExpectedDriveEntries.back();
			if (expectingAssignment)
				expectedEntry.replicators().emplace(Replicator_Key);
			if (actualReplicatorCount < replicatorCount)
				values.InitialDriveQueue.emplace(key, utils::CalculateDrivePriority(initialEntry, Min_Replicator_Count));
			if ((!expectingAssignment && actualReplicatorCount < replicatorCount) || actualReplicatorCount + 1 < replicatorCount)
				values.ExpectedDriveQueue.emplace(key, utils::CalculateDrivePriority(expectedEntry, Min_Replicator_Count));
		}

		void PrepareTestValues(TestValues& values) {
			AddDriveToTestValues(values, Drive1_Key, Drive1_Size, Drive1_Replicator_Count, Drive1_Actual_Replicator_Count, true);
			AddDriveToTestValues(values, Drive2_Key, Drive2_Size, Drive2_Replicator_Count, Drive2_Actual_Replicator_Count, false);
			AddDriveToTestValues(values, Drive3_Key, Drive3_Size, Drive3_Replicator_Count, Drive3_Actual_Replicator_Count, false);

			values.ExpectedReplicatorEntry.setCapacity(Capacity);
			values.ExpectedReplicatorEntry.drives().emplace(Drive1_Key, state::DriveInfo{ Hash256(), false, 0 });
		}

        void RunTest(NotifyMode mode, TestValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight, CreateConfig());
            Notification notification(Replicator_Key, Capacity);
			auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();
			auto& driveCache = context.cache().sub<cache::BcDriveCache>();

			//Populate cache
			test::AddAccountState(accountCache, Replicator_Key, Current_Height,
					{{Storage_Mosaic_Id, Capacity}, {Streaming_Mosaic_Id, Amount(2 * Capacity.unwrap())}});
			for (const auto& driveEntry : values.InitialDriveEntries) {
				driveCache.insert(driveEntry);
				test::AddAccountState(accountCache, driveEntry.key(), Current_Height);
			}

			auto pDriveQueue = std::make_unique<DriveQueue>(values.InitialDriveQueue);
			auto pObserver = CreateReplicatorOnboardingObserver(pDriveQueue);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
     		auto replicatorIter = replicatorCache.find(values.ExpectedReplicatorEntry.key());
			const auto& actualReplicatorEntry = replicatorIter.get();
			test::AssertEqualReplicatorData(values.ExpectedReplicatorEntry, actualReplicatorEntry);
			for (const auto& driveEntry : values.ExpectedDriveEntries) {
				auto driveIter = driveCache.find(driveEntry.key());
				auto& actualDriveEntry = driveIter.get();
				test::AssertEqualBcDriveData(driveEntry, actualDriveEntry);
			}

			// Check drive queue
			EXPECT_EQ(pDriveQueue->size(), values.ExpectedDriveQueue.size());
			while (!pDriveQueue->empty()) {
				EXPECT_EQ(pDriveQueue->top(), values.ExpectedDriveQueue.top());
				pDriveQueue->pop();
				values.ExpectedDriveQueue.pop();
			}
        }
    }

    TEST(TEST_CLASS, ReplicatorOnboarding_Commit) {
        // Arrange:
        TestValues values;
		PrepareTestValues(values);

        // Assert:
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, ReplicatorOnboarding_Rollback) {
        // Arrange:
		TestValues values;

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}