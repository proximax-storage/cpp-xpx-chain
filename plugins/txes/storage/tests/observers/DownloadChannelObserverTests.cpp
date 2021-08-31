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

        const Key Consumer = test::GenerateRandomByteArray<Key>();
        constexpr Amount Transaction_Fee(100);
        constexpr auto Download_Size = 100;
        constexpr Amount Storage_Units(Download_Size);
        constexpr Height Current_Height(15);

        struct CacheValues {
            public:
                explicit CacheValues()
                    : InitialBcDriveEntry(Key())
			    	, ExpectedBcDriveEntry(Key())
                    , DownloadChannelEntries(Hash256())
                {}
            
            public:
                state::DownloadChannelEntry DownloadChannelEntries;
                state::BcDriveEntry InitialBcDriveEntry;
			    state::BcDriveEntry ExpectedBcDriveEntry;
        };

        state::DownloadChannelEntry CreateDownloadChannelEntry(const Hash256& id) {
            state::DownloadChannelEntry entry(id);
            entry.setConsumer(Consumer);

            return entry;
        }

        state::BcDriveEntry CreateInitialBcDriveEntry(const Key& driveKey) {
            state::BcDriveEntry entry(driveKey);

            return entry;
        }

        state::BcDriveEntry CreateExpectedBcDriveEntry(const Key& driveKey, const Hash256& id) {
            state::BcDriveEntry entry(driveKey);

            return entry;
        }

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
			ObserverTestContext context(mode, currentHeight);
			Notification notification(
                values.DownloadChannelEntries.id(),
                values.DownloadChannelEntries.consumer(), 
                Download_Size,
				0u,
				nullptr);
            auto pObserver = CreateDownloadChannelObserver();
            auto& downloadChannelCache = context.cache().sub<cache::DownloadChannelCache>();
            auto& driveCache = context.cache().sub<cache::BcDriveCache>(); 

            // Populate cache.
            downloadChannelCache.insert(values.DownloadChannelEntries);
            driveCache.insert(values.InitialBcDriveEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto downloadChannelIter = downloadChannelCache.find(values.DownloadChannelEntries.id());
            const auto& actualDownloadChannelEntry = downloadChannelIter.get();
            test::AssertEqualDownloadChannelData(values.DownloadChannelEntries, actualDownloadChannelEntry);
          
            auto driveIter = driveCache.find(values.InitialBcDriveEntry.key());
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(values.ExpectedBcDriveEntry, actualEntry);
        }
    }

    TEST(TEST_CLASS, DownloadChannel_Commit) {
        // Arrange:
        CacheValues values;
        auto driveKey = test::GenerateRandomByteArray<Key>();
        auto activeDownloadId = test::GenerateRandomByteArray<Hash256>();
        values.InitialBcDriveEntry = CreateInitialBcDriveEntry(driveKey);  
        values.DownloadChannelEntries = CreateDownloadChannelEntry(activeDownloadId);
        values.ExpectedBcDriveEntry = CreateExpectedBcDriveEntry(driveKey, activeDownloadId);

        // Assert:
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, DownloadChannel_Rollback) {
        // Arrange:
        CacheValues values;

        // Assert:
        EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}