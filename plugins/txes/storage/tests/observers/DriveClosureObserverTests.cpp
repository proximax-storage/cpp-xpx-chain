/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
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

        state::BcDriveEntry CreateInitialBcDriveEntry(const Key& driveKey, const utils::KeySet& replicatorKeys){
            state::BcDriveEntry entry(driveKey);
            entry.setSize(Drive_Size);
            entry.setReplicatorCount(Num_Replicators);
			entry.replicators() = replicatorKeys;

            return entry;
        }

		state::ReplicatorEntry CreateInitialReplicatorEntry(const Key& driveKey, const Key& replicatorKey){
			state::ReplicatorEntry entry(replicatorKey);
			entry.drives().emplace(driveKey);

			return entry;
		}

        state::ReplicatorEntry CreateExpectedReplicatorEntry(const Key& replicatorKey){
            state::ReplicatorEntry entry(replicatorKey);

            return entry;
        }

        struct CacheValues {
		public:
			CacheValues()
				: InitialBcDriveEntry(Key())
				, ExpectedBcDriveEntry(Key())
			{}

		public:
			state::BcDriveEntry InitialBcDriveEntry;
			state::BcDriveEntry ExpectedBcDriveEntry;
			std::vector<state::ReplicatorEntry> InitialReplicatorEntries;
			std::vector<state::ReplicatorEntry> ExpectedReplicatorEntries;
		};

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height);
            Notification notification(values.InitialBcDriveEntry.key());
            auto pObserver = CreateDriveClosureObserver();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();

            // Populate cache.
            bcDriveCache.insert(values.InitialBcDriveEntry);
            for (const auto& entry : values.InitialReplicatorEntries)
        		replicatorCache.insert(entry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			EXPECT_FALSE(bcDriveCache.find(values.ExpectedBcDriveEntry.key()).tryGet());

            for (const auto& entry : values.ExpectedReplicatorEntries) {
				auto replicatorIter = replicatorCache.find(entry.key());
				const auto &actualEntry = replicatorIter.get();
				test::AssertEqualReplicatorData(entry, actualEntry);
			}
        }
    }

    TEST(TEST_CLASS, DriveClosure_Commit) {
        // Arrange:
        CacheValues values;
        auto driveKey = test::GenerateRandomByteArray<Key>();
        utils::KeySet replicatorKeys;
        for (auto i = 0u; i < Num_Replicators; ++i) {
        	auto replicatorKey = test::GenerateRandomByteArray<Key>();
			replicatorKeys.emplace(replicatorKey);
			values.InitialReplicatorEntries.push_back(CreateInitialReplicatorEntry(driveKey, replicatorKey));
			values.ExpectedReplicatorEntries.push_back(CreateExpectedReplicatorEntry(replicatorKey));
		}
		values.InitialBcDriveEntry = CreateInitialBcDriveEntry(driveKey, replicatorKeys);

        // Assert
		RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, DriveClosure_Rollback) {
        // Arrange:
        CacheValues values;

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}