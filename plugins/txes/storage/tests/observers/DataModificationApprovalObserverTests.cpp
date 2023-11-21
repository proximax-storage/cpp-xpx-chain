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
		constexpr uint8_t Modification_Status = 0;
        constexpr auto File_Structure_Size = 50;
		constexpr auto Meta_Files_Size = 50;
        constexpr auto Used_Drive_Size = 50;
		constexpr auto Judging_Keys_Count = 2;
		constexpr auto Overlapping_Keys_Count = 2;
		constexpr auto Judged_Keys_Count = 2;

        state::BcDriveEntry CreateInitialEntry(const Key& driveKey) {
            state::BcDriveEntry entry(driveKey);
            entry.activeDataModifications().emplace_back(state::ActiveDataModification {
                test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(), 
                test::GenerateRandomByteArray<Hash256>(), test::Random()
            });
            
            return entry;
        }

        state::BcDriveEntry CreateExpectedEntry(const Key& driveKey, const state::ActiveDataModifications& activeDataModifications) {
            state::BcDriveEntry entry(driveKey);
            for (const auto &activeDataModification: activeDataModifications)
                entry.completedDataModifications().emplace_back(activeDataModification, state::DataModificationApprovalState::Approved, 0U);

			const auto totalJudgingKeysCount = Judging_Keys_Count + Overlapping_Keys_Count;
			for (auto i = 0u; i < totalJudgingKeysCount; ++i)
				entry.confirmedUsedSizes().insert({test::GenerateRandomByteArray<Key>(), Used_Drive_Size});

			entry.setRootHash(entry.completedDataModifications().back().DownloadDataCdi);

            return entry;
        }

        struct CacheValues {
            public:
                explicit CacheValues()
                    : InitialBcDriveEntry(Key())
                    , ExpectedBcDriveEntry(Key())
                {}
            
            public:
                state::BcDriveEntry InitialBcDriveEntry;
			    state::BcDriveEntry ExpectedBcDriveEntry;
        };

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
			const auto& confirmedUsedSizes = values.ExpectedBcDriveEntry.confirmedUsedSizes();
			auto pPublicKeys = std::make_unique<Key[]>(confirmedUsedSizes.size());
			auto pPair = confirmedUsedSizes.begin();
			for (auto i = 0u; i < confirmedUsedSizes.size(); ++i, ++pPair)
				pPublicKeys[i] = pPair->first;
			const auto pPresentOpinions = std::make_unique<uint8_t>();

            ObserverTestContext context(mode, currentHeight);
            Notification notification(
            	values.InitialBcDriveEntry.key(),
                values.InitialBcDriveEntry.activeDataModifications().begin()->Id, 
                values.InitialBcDriveEntry.activeDataModifications().begin()->DownloadDataCdi,
                Modification_Status,
                File_Structure_Size,
				Meta_Files_Size,
				Used_Drive_Size,
				Judging_Keys_Count,
				Overlapping_Keys_Count,
				Judged_Keys_Count,
				pPublicKeys.get(),
				pPresentOpinions.get());
            auto pObserver = CreateDataModificationApprovalObserver();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();

            // Populate cache.
            bcDriveCache.insert(values.InitialBcDriveEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto driveIter = bcDriveCache.find(values.InitialBcDriveEntry.key());
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(values.ExpectedBcDriveEntry, actualEntry);
        }
    }

//    TEST(TEST_CLASS, DataModificationApproval_Commit) {
//        // Arrange:
//        CacheValues values;
//        const auto driveKey = test::GenerateRandomByteArray<Key>();
//        values.InitialBcDriveEntry = CreateInitialEntry(driveKey);
//        values.ExpectedBcDriveEntry = CreateExpectedEntry(driveKey, values.InitialBcDriveEntry.activeDataModifications());
//
//        // Assert
//        RunTest(NotifyMode::Commit, values, Current_Height);
//    }

    TEST(TEST_CLASS, DataModificationApproval_Rollback) {
        // Arrange:
        CacheValues values;
        const auto driveKey = test::GenerateRandomByteArray<Key>();
        values.ExpectedBcDriveEntry = CreateInitialEntry(driveKey);
        values.InitialBcDriveEntry = values.ExpectedBcDriveEntry;

        // Assert
        EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }

}}