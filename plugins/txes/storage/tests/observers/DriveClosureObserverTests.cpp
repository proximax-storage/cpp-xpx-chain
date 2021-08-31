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
        constexpr auto Num_Active_Downloads = 5;

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

        state::DownloadChannelEntry CreateInitialDownloadChannelEntry(const Hash256& id){
            state::DownloadChannelEntry entry(id);

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
            std::vector<state::DownloadChannelEntry> InitialDownloadChannelEntries;
		};

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height);
            Notification notification(values.InitialBcDriveEntry.key());
            auto pObserver = CreateDriveClosureObserver();
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
            auto& downloadChannelCache = context.cache().sub<cache::DownloadChannelCache>();

            // Populate cache.
            bcDriveCache.insert(values.InitialBcDriveEntry);
            for (const auto& entry : values.InitialReplicatorEntries)
        		replicatorCache.insert(entry);
            for (const auto& entry : values.InitialDownloadChannelEntries)
                downloadChannelCache.insert(entry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			EXPECT_FALSE(bcDriveCache.find(values.ExpectedBcDriveEntry.key()).tryGet());

            for (const auto& entry : values.ExpectedReplicatorEntries) {
				auto replicatorIter = replicatorCache.find(entry.key());
				const auto &actualEntry = replicatorIter.get();
				test::AssertEqualReplicatorData(entry, actualEntry);
			}

            for (const auto& entry : values.InitialDownloadChannelEntries)
                EXPECT_FALSE(downloadChannelCache.find(entry.id()).tryGet());
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
        for (auto j = 0u; j < Num_Active_Downloads; j++) {
            auto activeDownloadId = test::GenerateRandomByteArray<Hash256>();
            values.InitialDownloadChannelEntries.push_back(CreateInitialDownloadChannelEntry(activeDownloadId));
			values.InitialBcDriveEntry.activeDownloads().push_back(activeDownloadId);
        }

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