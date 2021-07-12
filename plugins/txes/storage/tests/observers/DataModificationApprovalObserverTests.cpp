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

#define TEST_CLASS DataModificationApprovalObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(DataModificationApproval,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::DataModificationApprovalNotification<1>;

        constexpr auto Current_Height = Height(10);
        const Hash256 Data_Modification_Id =  test::GenerateRandomByteArray<Hash256>();
        const Hash256 File_Structure_Cdi =  test::GenerateRandomByteArray<Hash256>();
        constexpr auto File_Structure_Size = 50;
        constexpr auto Used_Drive_Size = 500;

        state::BcDriveEntry CreateInitialEntry() {
            state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
            entry.activeDataModifications().emplace_back(state::ActiveDataModification{
                Data_Modification_Id,
                test::GenerateRandomByteArray<Key>(),
                test::GenerateRandomByteArray<Hash256>(),
                test::Random()
            });

            return entry;
        }

        state::BcDriveEntry CreateExpectedEntry(state::BcDriveEntry& initialEntry) {
            state::BcDriveEntry entry(initialEntry.key());
            auto& completedDataModifications = entry.completedDataModifications();
            auto& activeDataModifications = entry.activeDataModifications();
            completedDataModifications.emplace_back(*activeDataModifications.begin(), state::DataModificationState::Succeeded);
		    activeDataModifications.erase(activeDataModifications.begin());

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

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(NotifyMode::Commit, Current_Height);
            Notification notification(values.InitialDriveEntry.key(), Data_Modification_Id, File_Structure_Cdi, File_Structure_Size, Used_Drive_Size);
            auto pObserver = CreateDataModificationApprovalObserver();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

            // Populate cache.
            bcDriveCache.insert(values.InitialDriveEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto driveIter = bcDriveCache.find(values.ExpectedDriveEntry.key());
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(values.ExpectedDriveEntry, actualEntry);

            EXPECT_TRUE(bcDriveCache.contains(values.ExpectedDriveEntry.key()));
        }
    }

    TEST(TEST_CLASS, DataModificationApproval_Commit) {
        // Arrange:
        CacheValues values;
        values.InitialDriveEntry = CreateInitialEntry();
        values.ExpectedDriveEntry = CreateExpectedEntry(values.InitialDriveEntry);

        // Assert
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, DataModificationCancel_Rollback) {
        // Arrange:
        CacheValues values;
		values.InitialDriveEntry = CreateInitialEntry();
		values.ExpectedDriveEntry = CreateExpectedEntry(values.InitialDriveEntry);

        // Assert
        RunTest(NotifyMode::Rollback, values, Current_Height);
    }

}}