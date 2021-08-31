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

#define TEST_CLASS PrepareDriveObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(PrepareDrive, std::make_shared<cache::ReplicatorKeyCollector>())

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::PrepareDriveNotification<1>;

        const Key Drive_Key = test::GenerateRandomByteArray<Key>();
        const Key Owner = test::GenerateRandomByteArray<Key>();
        constexpr auto Drive_Size = 50;
        constexpr auto Replicator_Count = 10;
        const auto Replicator_Key_Collector = std::make_shared<cache::ReplicatorKeyCollector>();
        constexpr Height Current_Height(20);
        
        state::BcDriveEntry CreateBcDriveEntry() {
            state::BcDriveEntry entry(Drive_Key);
            entry.setOwner(Owner);
			entry.setSize(Drive_Size);
            entry.setReplicatorCount(Replicator_Count);

            return entry;
        }

        state::ReplicatorEntry CreateReplicatorEntry(const Key& driveKey, const std::shared_ptr<cache::ReplicatorKeyCollector>& replicatorKeyCollector) {
            state::ReplicatorEntry entry(driveKey);
            replicatorKeyCollector->addKey(entry);
            entry.drives().emplace(*replicatorKeyCollector->keys().begin(), state::DriveInfo());
            
            return entry;
        }

        struct CacheValues {
            public:
			    explicit CacheValues()
                    : BcDriveEntry(Key())
                    , ReplicatorEntry(Key())
                {}

            public:
                state::BcDriveEntry BcDriveEntry;
                state::ReplicatorEntry ReplicatorEntry;
        };
        
        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            ObserverTestContext context(mode, currentHeight);
            Notification notification(
                values.BcDriveEntry.owner(),
                values.ReplicatorEntry.key(),
                values.BcDriveEntry.size(),
                values.BcDriveEntry.replicatorCount());
            auto pObserver = CreatePrepareDriveObserver(Replicator_Key_Collector);
            auto& driveCache = context.cache().sub<cache::BcDriveCache>();
            auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();

            // Populate cache.
            replicatorCache.insert(values.ReplicatorEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto driveIter = driveCache.find(values.BcDriveEntry.key());
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(values.BcDriveEntry, actualEntry);

            auto replicatorIter = replicatorCache.find(values.ReplicatorEntry.key());
			auto& replicatorEntry = replicatorIter.get();
            test::AssertEqualReplicatorData(values.ReplicatorEntry, replicatorEntry);
        }
    }

    TEST(TEST_CLASS, PrepareDrive_Commit) {
        // Arrange:
        CacheValues values;
        values.BcDriveEntry = CreateBcDriveEntry();
        values.ReplicatorEntry = CreateReplicatorEntry(values.BcDriveEntry.key(), Replicator_Key_Collector);

        // Assert:
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, PrepareDrive_Rollback) {
        // Arrange:
        CacheValues values;

        // Assert:
        EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }

}}