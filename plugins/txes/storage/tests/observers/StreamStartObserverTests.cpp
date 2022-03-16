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

#define TEST_CLASS StreamStartObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(StreamStart,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::StreamStartNotification<1>;

        constexpr auto Current_Height = Height(10);

        struct BcDriveValues {
            public:
                explicit BcDriveValues(
						const Key& key,
						const state::ActiveDataModification& modification)
                    : Drive_Key(key)
                    , Active_Data_Modifications {
						modification
					}
                {}
            
            public:
                Key Drive_Key;
                std::vector<state::ActiveDataModification> Active_Data_Modifications;
        };

        state::BcDriveEntry CreateEntry(const BcDriveValues& values) {
            state::BcDriveEntry entry(values.Drive_Key);
            for (const auto &activeDataModification : values.Active_Data_Modifications) {
                entry.activeDataModifications().emplace_back(activeDataModification);
            }

            return entry;
        }

		auto RandomBcDriveValues() {
			auto key = test::GenerateRandomByteArray<Key>();
			auto expectedUploadSize = test::Random();
			auto folderNameBytes = test::GenerateRandomVector(512);
			auto modification = state::ActiveDataModification (
				test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(),
				expectedUploadSize, std::string(folderNameBytes.begin(), folderNameBytes.end()));
			return BcDriveValues(key, modification);
		}

        void RunTest(NotifyMode mode, const BcDriveValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight);
            Notification notification(
                values.Active_Data_Modifications.begin()->Id,
                values.Drive_Key, 
                values.Active_Data_Modifications.begin()->Owner,
                values.Active_Data_Modifications.begin()->ExpectedUploadSizeMegabytes,
				values.Active_Data_Modifications.begin()->FolderName);
            auto pObserver = CreateStreamStartObserver();
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

    TEST(TEST_CLASS, StreamStart_Commit) {
        // Arrange:
        auto values = RandomBcDriveValues();
        
        // Assert:
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, StreamStart_Rollback) {
        // Arrange:
		auto values = RandomBcDriveValues();

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}