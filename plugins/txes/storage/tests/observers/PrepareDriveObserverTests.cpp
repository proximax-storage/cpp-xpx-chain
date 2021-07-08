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

        constexpr auto Replicator_Key_Collector = ReplicatorKeyCollector(1234);
        constexpr Height Current_Height(20);
        
        struct BcDriveValues {
            public:
			explicit BcDriveValues()
                : Owner(test::GenerateRandomByteArray<Key>())
                , DriveKey(test::GenerateRandomByteArray<Key>())
                , DriveSize(test::Random())
                , ReplicatorCount(test::Random16())
            {}

            public:
                Key Owner;
                Key DriveKey;
                uint64_t DriveSize;
                uint16_t ReplicatorCount;
        };
        
        state::BcDriveEntry CreateBcDriveEntry(const BcDriveValues& values){
            state::BcDriveEntry entry(values.DriveKey);
            return entry;
        }
    }

    TEST(TEST_CLASS, PrepareDrive_Commit) {
        // Arrange:
        ObserverTestContext context(NotifyMode::Commit, Current_Height, Replicator_Key_Collector);
        BcDriveValues values;
        Notification notification(values.Owner, values.DriveKey, values.DriveSize, values.ReplicatorCount);
        auto pObserver = CreatePrepareDriveObserver(Replicator_Key_Collector);

        auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

        // Populate cache.
        bcDriveCache.insert(CreateBcDriveEntry(values));

        // Act:
        test::ObserveNotification(*pObserver, notification, context);

        // Assert: check the cache
        auto driveIter = bcDriveCache.find(values.DriveKey);
        const auto& actualEntry = driveIter.get();
        test::AssertEqualBcDriveData(CreateBcDriveEntry(values), actualEntry);
    }

    TEST(TEST_CLASS, PrepareDrive_Rollback) {
        // Arrange:
        ObserverTestContext context(NotifyMode::Rollback, Current_Height, Replicator_Key_Collector);
        BcDriveValues values;
        Notification notification(values.Owner, values.DriveKey, values.DriveSize, values.ReplicatorCount);
        auto pObserver = CreatePrepareDriveObserver(Replicator_Key_Collector);

        auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

        // Populate cache.
        bcDriveCache.insert(CreateBcDriveEntry(values));

        // Act:
        test::ObserveNotification(*pObserver, notification, context);

        // Assert: check the cache
        auto driveIter = bcDriveCache.find(values.DriveKey);
        const auto& actualEntry = driveIter.get();
        test::AssertEqualBcDriveData(CreateBcDriveEntry(values), actualEntry);
    }

}}