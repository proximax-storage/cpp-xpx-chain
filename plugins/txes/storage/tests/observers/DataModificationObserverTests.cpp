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

    DEFINE_COMMON_OBSERVER_TESTS(DataModification,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::DataModificationNotification<1>;

        constexpr auto Current_Height = Height(10);

        struct BcDriveValues {
            public:
                explicit BcDriveValues()
                    : DriveKey(test::GenerateRandomByteArray<Key>())
                    , DataModificationId(test::GenerateRandomByteArray<Hash256>())
                    , Owner(test::GenerateRandomByteArray<Key>())
                    , DownloadDataCdi(test::GenerateRandomByteArray<Hash256>())
                    , UploadSize(test::Random())
                {}
            
            public:
                Key DriveKey;
                Hash256 DataModificationId;
                Key Owner;
                Hash256 DownloadDataCdi;
                uint64_t UploadSize;
                std::vector<state::ActiveDataModification> ActiveDataModification;
        };

        state::BcDriveEntry CreateEntry(const BcDriveValues& values) {
            state::BcDriveEntry entry(values.DriveKey);
            entry.activeDataModifications().emplace_back(state::ActiveDataModification{
                values.DataModificationId,
                values.Owner,
                values.DownloadDataCdi,
                values.UploadSize
            });

            return entry;
        }
    }

    TEST(TEST_CLASS, DataModification_Commit) {
        // Arrange:
        ObserverTestContext context(NotifyMode::Commit, Current_Height);
        BcDriveValues values;
        Notification notification(values.DataModificationId, values.DriveKey, values.Owner, values.DownloadDataCdi, values.UploadSize);
        auto pObserver = CreateDataModificationObserver();
        auto& driveCache = context.cache().sub<cache::BcDriveCache>();

        // Populate cache.
        driveCache.insert(CreateEntry(values));

        // Act:
        test::ObserveNotification(*pObserver, notification, context);

        // Assert:
        auto driveIter = driveCache.find(values.DriveKey);
        auto& actualEntry = driveIter.get();
        test::AssertEqualBcDriveData(CreateEntry(values), actualEntry);
    }

    TEST(TEST_CLASS, DataModification_Rollback) {
        // Arrange:
        ObserverTestContext context(NotifyMode::Rollback, Current_Height);
        BcDriveValues values;
        Notification notification(values.DataModificationId, values.DriveKey, values.Owner, values.DownloadDataCdi, values.UploadSize);
        auto pObserver = CreateDataModificationObserver();
        auto& driveCache = context.cache().sub<cache::BcDriveCache>();

        // Act:
        test::ObserveNotification(*pObserver, notification, context);

        // Assert:
        EXPECT_FALSE(driveCache.contains(values.DriveKey));
    }
}}