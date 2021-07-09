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

#define TEST_CLASS DownloadChannelObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(DownloadChannel,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::DownloadChannelCacheFactory>;
        using Notification = model::DownloadNotification<1>;

        const Hash256 Id = test::GenerateRandomByteArray<Hash256>();
        const Key Consumer = test::GenerateRandomByteArray<Key>();
        const Key Drive = test::GenerateRandomByteArray<Key>();
        constexpr Amount Transaction_Fee(100);
        constexpr auto Download_Size = 100;
        constexpr Amount Storage_Units(Download_Size);
        constexpr Height Current_Height(15);

        state::DownloadChannelEntry CreateDownloadChannelEntry() {
            state::DownloadChannelEntry entry(Id);
            entry.setConsumer(Consumer);
            entry.setDrive(Drive);
            entry.setTransactionFee(Transaction_Fee);
            entry.setStorageUnits(Storage_Units);

            return entry;
        }

        state::BcDriveEntry CreateBcDriveEntry(const Key& drive, const Hash256& id) {
            state::BcDriveEntry entry(drive);
            entry.activeDownloads().emplace_back(id);

            return entry;
        }

        struct CacheValues {
            public:
                explicit CacheValues()
                    : DownloadChannelEntry(Hash256())
                    , BcDriveEntry(Key())
                {}
            
            public:
                state::DownloadChannelEntry DownloadChannelEntry;
                state::BcDriveEntry BcDriveEntry;
        };

        void RunTest(NotifyMode mode, const CacheValues& values, const uint64_t& downloadSize) {
            // Arrange:
			ObserverTestContext context(mode, Current_Height);
			Notification notification(
                values.BcDriveEntry.activeDownloads().back(), 
                values.DownloadChannelEntry.drive(), 
                values.DownloadChannelEntry.consumer(), 
                downloadSize, 
                values.DownloadChannelEntry.storageUnits());
            auto pObserver = CreateDownloadChannelObserver();
            auto& downloadChannelCache = context.cache().sub<cache::DownloadChannelCache>();
            auto& driveCache = context.cache().sub<cache::BcDriveCache>();

            // Populate cache.
            downloadChannelCache.insert(values.DownloadChannelEntry);
            driveCache.insert(values.BcDriveEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto downloadChannelIter = downloadChannelCache.find(values.BcDriveEntry.activeDownloads().back());
            const auto& actualDownloadChannelEntry = downloadChannelIter.get();
            test::AssertEqualDownloadChannelData(values.DownloadChannelEntry, actualDownloadChannelEntry);

            auto driveIter = driveCache.find(values.BcDriveEntry.key());
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(values.BcDriveEntry, actualEntry);
        }
    }

    TEST(TEST_CLASS, DownloadChannel_Commit) {
        // Arrange:
        CacheValues values;
        values.DownloadChannelEntry = CreateDownloadChannelEntry();
        values.BcDriveEntry = CreateBcDriveEntry(values.DownloadChannelEntry.drive(), Id);

        // Assert:
        RunTest(NotifyMode::Commit, values, Download_Size);
    }

    TEST(TEST_CLASS, DownloadChannel_Rollback) {
        // Arrange:
        CacheValues values;
        values.DownloadChannelEntry = CreateDownloadChannelEntry();
        values.BcDriveEntry = CreateBcDriveEntry(values.DownloadChannelEntry.drive(), Id);

        // Assert:
        RunTest(NotifyMode::Rollback, values, Download_Size);
    }
}}