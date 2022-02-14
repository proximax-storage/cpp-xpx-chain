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

        state::BcDriveEntry CreateEntry(const BcDriveValues& values) {
            state::BcDriveEntry entry(values.Drive_Key);
            for (const auto &activeDataModification : values.Active_Data_Modification) {
                entry.activeDataModifications().emplace_back(activeDataModification);
            }

            return entry;
        }

        void RunTest(NotifyMode mode, const BcDriveValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight);
            Notification notification(
                values.Active_Data_Modification.begin()->Id, 
                values.Drive_Key, 
                values.Active_Data_Modification.begin()->Owner, 
                values.Active_Data_Modification.begin()->DownloadDataCdi, 
                values.Active_Data_Modification.begin()->ExpectedUploadSizeMegabytes);
            auto pObserver = CreateDataModificationObserver();
        	auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

            // Populate cache.
            bcDriveCache.insert(state::BcDriveEntry(values.Drive_Key));

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto driveIter = bcDriveCache.find(values.Drive_Key);
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(CreateEntry(values), actualEntry);
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