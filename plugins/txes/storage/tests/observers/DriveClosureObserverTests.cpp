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

#define TEST_CLASS DriveClosureObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(DriveClosure,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::DriveClosureNotification<1>;

        constexpr Height Current_Height(20);
        constexpr auto Drive_Size = 100;
        constexpr auto Num_Replicators = 10;

        state::BcDriveEntry CreateInitialBcDriveEntry(){
            state::BcDriveEntry entry(test::GenerateRandomByteArray<Key>());
            entry.setSize(Drive_Size);
            entry.setReplicatorCount(Num_Replicators);

            return entry;
        }

        state::BcDriveEntry CreateExpectedBcDriveEntry(state::BcDriveEntry& initialEntry){
            state::BcDriveEntry entry(initialEntry);
            entry.setSize(0);
            entry.setReplicatorCount(0);

            return entry;
        }

        struct CacheValues {
		public:
			CacheValues() : InitialBcDriveEntry(Key()), ExpectedBcDriveEntry(Key())
			{}

		public:
			state::BcDriveEntry InitialBcDriveEntry;
			state::BcDriveEntry ExpectedBcDriveEntry;
		};

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(NotifyMode::Commit, Current_Height);
            Notification notification(values.InitialBcDriveEntry.key());
            auto pObserver = CreateDriveClosureObserver();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

            // Populate cache.
            bcDriveCache.insert(values.InitialBcDriveEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto driveIter = bcDriveCache.find(values.ExpectedBcDriveEntry.key());
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(values.ExpectedBcDriveEntry, actualEntry);
        }
    }

    TEST(TEST_CLASS, DriveClosure_Commit) {
        // Arrange:
        CacheValues values;
		values.InitialBcDriveEntry = CreateInitialBcDriveEntry();
		values.ExpectedBcDriveEntry = CreateExpectedBcDriveEntry(values.InitialBcDriveEntry);

        // Assert
		RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, DriveClosure_Rollback) {
        // Arrange:
        CacheValues values;
		values.InitialBcDriveEntry = CreateInitialBcDriveEntry();
		values.ExpectedBcDriveEntry = values.InitialBcDriveEntry;

        // Assert
		RunTest(NotifyMode::Rollback, values, Current_Height);
    }
}}