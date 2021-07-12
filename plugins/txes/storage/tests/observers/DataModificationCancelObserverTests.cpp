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

#define TEST_CLASS DataModificationCancelObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(DataModificationCancel,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::DataModificationCancelNotification<1>;

        constexpr auto Current_Height = Height(10);

        state::BcDriveEntry CreateInitialEntry() {
            state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
            entry.activeDataModifications().emplace_back(state::ActiveDataModification{
                test::GenerateRandomByteArray<Hash256>(),
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Hash256>(),
                test::Random()
            });

            return entry;
        }

        state::BcDriveEntry CreateExpectedEntry(state::BcDriveEntry& initialEntry) {
            state::BcDriveEntry entry(initialEntry.key());
            auto& activeDataModifications = entry.activeDataModifications();
            auto i = 0;
            for (auto it = ++activeDataModifications.begin(); it != activeDataModifications.end(); ++it) {
                if (it->Id == activeDataModifications[i].Id) {
                    activeDataModifications.erase(it);
                    entry.completedDataModifications().emplace_back(state::CompletedDataModification(*it, state::DataModificationState::Cancelled));
                    break;
                }
                i++;
            }

            return entry;
        }

        struct CacheValues {
        public:
            CacheValues() : InitialDriveEntry(Key()), ExpectedDriveEntry(Key())
            {}

        public:
            state::BcDriveEntry InitialDriveEntry;
            state::BcDriveEntry ExpectedDriveEntry;
        };

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight, const u_int8_t no) {
            // Arrange:
            ObserverTestContext context(NotifyMode::Commit, Current_Height);
            Notification notification(values.InitialDriveEntry.key(), values.InitialDriveEntry.owner(), values.InitialDriveEntry.activeDataModifications()[no].Id);
            auto pObserver = CreateDataModificationCancelObserver();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

            // Populate cache.
            bcDriveCache.insert(values.InitialDriveEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto driveIter = bcDriveCache.find(values.ExpectedDriveEntry.key());
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(values.ExpectedDriveEntry, actualEntry);
        }
    }
    
    TEST(TEST_CLASS, DataModificationCancel_Commit) {
        // Arrange:
        CacheValues values;
        values.InitialDriveEntry = CreateInitialEntry();
        values.ExpectedDriveEntry = CreateExpectedEntry(values.InitialDriveEntry);

        // Assert
        for(auto i = 0; i < values.InitialDriveEntry.activeDataModifications().size(); ++i)
            RunTest(NotifyMode::Commit, values, Current_Height, i);
    }

    TEST(TEST_CLASS, DataModificationCancel_Rollback) {
        // Arrange:
        CacheValues values;
		values.InitialDriveEntry = CreateInitialEntry();
		values.ExpectedDriveEntry = CreateExpectedEntry(values.InitialDriveEntry);

        // Assert
        for(auto i = 0; i < values.InitialDriveEntry.activeDataModifications().size(); ++i)
    		RunTest(NotifyMode::Rollback, values, Current_Height, i);
    }
}}