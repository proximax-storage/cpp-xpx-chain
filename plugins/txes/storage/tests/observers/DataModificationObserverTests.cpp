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

#define TEST_CLASS DataModificationObserverTests

	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	DEFINE_COMMON_OBSERVER_TESTS(DataModification, std::make_shared<cache::ReplicatorKeyCollector>(), std::make_shared<DriveQueue>())

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::DataModificationNotification<1>;

        constexpr auto Current_Height = Height(10);
		const auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
		const auto Drive_Queue = std::make_shared<DriveQueue>();
		const auto Replicator_Key = test::GenerateRandomByteArray<Key>();
		const auto Offboarding_Replicator_Key = test::GenerateRandomByteArray<Key>();
		constexpr auto Min_Replicator_Count = 4;
		constexpr auto Shard_Size = 3;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::StorageConfiguration::Uninitialized();

			pluginConfig.MinReplicatorCount = Min_Replicator_Count;
			pluginConfig.ShardSize = Shard_Size;
			config.Network.SetPluginConfiguration(pluginConfig);

			return config.ToConst();
		}

        struct BcDriveValues {
            public:
                explicit BcDriveValues()
                    : Drive_Key(test::GenerateRandomByteArray<Key>())
                    , Active_Data_Modification {
                        state::ActiveDataModification(
                            test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(), 
                            test::GenerateRandomByteArray<Hash256>(), test::Random()
					)}
                {}
            
            public:
                Key Drive_Key;
                std::vector<state::ActiveDataModification> Active_Data_Modification;
        };

        state::BcDriveEntry CreateInitialEntry(const BcDriveValues& values) {
            state::BcDriveEntry entry(values.Drive_Key);

			entry.replicators() = { Replicator_Key, Offboarding_Replicator_Key };
			entry.offboardingReplicators() = { Offboarding_Replicator_Key };

            return entry;
        }

		state::BcDriveEntry CreateExpectedEntry(const BcDriveValues& values) {
			state::BcDriveEntry entry(values.Drive_Key);

			for (const auto &activeDataModification : values.Active_Data_Modification) {
				entry.activeDataModifications().emplace_back(activeDataModification);
			}
			entry.replicators() = { Replicator_Key };
			entry.offboardingReplicators() = {};

			return entry;
		}

        void RunTest(NotifyMode mode, const BcDriveValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight, CreateConfig());
            Notification notification(
                values.Active_Data_Modification.begin()->Id, 
                values.Drive_Key, 
                values.Active_Data_Modification.begin()->Owner, 
                values.Active_Data_Modification.begin()->DownloadDataCdi, 
                values.Active_Data_Modification.begin()->ExpectedUploadSizeMegabytes);
            auto pObserver = CreateDataModificationObserver(Replicator_Key_Collector, Drive_Queue);
        	auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
			auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			auto& accountCache = context.cache().sub<cache::AccountStateCache>();

            // Populate cache.
            bcDriveCache.insert(CreateInitialEntry(values));
			accountCache.addAccount(values.Drive_Key, Current_Height);
			for (const auto& replicatorKey : { Replicator_Key, Offboarding_Replicator_Key }) {
				replicatorCache.insert(test::CreateReplicatorEntry(replicatorKey));
				accountCache.addAccount(replicatorKey, Current_Height);
			}

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto driveIter = bcDriveCache.find(values.Drive_Key);
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(CreateExpectedEntry(values), actualEntry);
        }
    }

    TEST(TEST_CLASS, DataModification_Commit) {
        // Arrange:
        BcDriveValues values;
        values.Drive_Key = test::GenerateRandomByteArray<Key>();
        
        // Assert:
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, DataModification_Rollback) {
        // Arrange:
        BcDriveValues values;

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}