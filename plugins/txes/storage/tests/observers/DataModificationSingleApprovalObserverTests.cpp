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

#define TEST_CLASS DataModificationSingleApprovalObserverTests

    DEFINE_COMMON_OBSERVER_TESTS(DataModificationSingleApproval,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::DataModificationSingleApprovalNotification<1>;

        constexpr auto Current_Height = Height(10);
        constexpr auto Used_Drive_Size = 50;
		constexpr auto Public_Keys_Count = 5;

        state::BcDriveEntry CreateInitialBcDriveEntry(const Key& driveKey) {
            state::BcDriveEntry entry(driveKey);
			const auto replicatorKey = test::GenerateRandomByteArray<Key>();
			const auto driveOwnerKey = test::GenerateRandomByteArray<Key>();
			entry.replicators().insert(replicatorKey);
			entry.setOwner(driveOwnerKey);
            entry.completedDataModifications().emplace_back(state::CompletedDataModification {
					{
                		test::GenerateRandomByteArray<Hash256>(), driveOwnerKey,
                		test::GenerateRandomByteArray<Hash256>(), test::Random()
					},
					state::DataModificationState::Succeeded
            });
			entry.confirmedUsedSizes().emplace(
					replicatorKey,
					test::RandomInRange<uint64_t>(0, Used_Drive_Size-1)
 			);

            return entry;
        }

		state::ReplicatorEntry CreateInitialReplicatorEntry(const state::BcDriveEntry& driveEntry) {
			state::ReplicatorEntry entry(*driveEntry.replicators().begin());
			entry.drives().emplace(
					driveEntry.key(),
					state::DriveInfo {
						test::GenerateRandomByteArray<Hash256>(),
						static_cast<bool>(test::RandomByte() % 2),
						test::Random()
					}
 			);
			return entry;
		}

        state::BcDriveEntry CreateExpectedBcDriveEntry(state::BcDriveEntry entry) {
			entry.confirmedUsedSizes().begin()->second = Used_Drive_Size;
            return entry;
        }

		state::ReplicatorEntry CreateExpectedReplicatorEntry(state::ReplicatorEntry entry, const Hash256& dataModificationId) {
			auto& driveInfo = entry.drives().begin()->second;
			driveInfo.LastApprovedDataModificationId = dataModificationId;
			driveInfo.DataModificationIdIsValid = true;
			driveInfo.InitialDownloadWork = 0;
			return entry;
		}

        struct CacheValues {
            public:
                explicit CacheValues()
                    : InitialBcDriveEntry(Key())
					, InitialReplicatorEntry(Key())
                    , ExpectedBcDriveEntry(Key())
					, ExpectedReplicatorEntry(Key())
                {}
            
            public:
                state::BcDriveEntry InitialBcDriveEntry;
			    state::BcDriveEntry ExpectedBcDriveEntry;
				state::ReplicatorEntry InitialReplicatorEntry;
				state::ReplicatorEntry ExpectedReplicatorEntry;
        };

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
			const auto pPublicKeys = std::make_unique<Key>();
			const auto pOpinions = std::make_unique<uint64_t>();
            ObserverTestContext context(mode, currentHeight);
            Notification notification(
				values.InitialReplicatorEntry.key(),
				values.InitialBcDriveEntry.key(),
                values.InitialBcDriveEntry.completedDataModifications().end()->Id,
				Used_Drive_Size,
				Public_Keys_Count,
				pPublicKeys.get(),
				pOpinions.get());
            auto pObserver = CreateDataModificationSingleApprovalObserver();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
			auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();

            // Populate caches
            bcDriveCache.insert(values.InitialBcDriveEntry);
			replicatorCache.insert(values.InitialReplicatorEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the caches
            auto driveIter = bcDriveCache.find(values.InitialBcDriveEntry.key());
			auto replicatorIter = replicatorCache.find(values.InitialReplicatorEntry.key());
            const auto& actualDriveEntry = driveIter.get();
			const auto& actualReplicatorEntry = replicatorIter.get();
            test::AssertEqualBcDriveData(values.ExpectedBcDriveEntry, actualDriveEntry);
			test::AssertEqualReplicatorData(values.ExpectedReplicatorEntry, actualReplicatorEntry);
        }
    }

    TEST(TEST_CLASS, DataModificationSingleApproval_Commit) {
        // Arrange:
        CacheValues values;
        values.InitialBcDriveEntry = CreateInitialBcDriveEntry(test::GenerateRandomByteArray<Key>());
		values.InitialReplicatorEntry = CreateInitialReplicatorEntry(values.InitialBcDriveEntry);
        values.ExpectedBcDriveEntry = CreateExpectedBcDriveEntry(values.InitialBcDriveEntry);
		values.ExpectedReplicatorEntry = CreateExpectedReplicatorEntry(
				values.InitialReplicatorEntry,
				values.InitialBcDriveEntry.completedDataModifications().end()->Id
		);

        // Assert
        RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, DataModificationSingleApproval_Rollback) {
        // Arrange:
		CacheValues values;
		values.ExpectedBcDriveEntry = CreateInitialBcDriveEntry(test::GenerateRandomByteArray<Key>());
		values.ExpectedReplicatorEntry = CreateInitialReplicatorEntry(values.ExpectedBcDriveEntry);
		values.InitialBcDriveEntry = CreateExpectedBcDriveEntry(values.ExpectedBcDriveEntry);
		values.InitialReplicatorEntry = CreateExpectedReplicatorEntry(
				values.ExpectedReplicatorEntry,
				values.ExpectedBcDriveEntry.completedDataModifications().end()->Id
		);

        // Assert
        EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }

}}