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
        using ObserverTestContext = test::ObserverTestContextT<test::StorageCacheFactory>;
        using Notification = model::DownloadNotification<1>;

        const Key Consumer = test::GenerateRandomByteArray<Key>();
        constexpr Amount Transaction_Fee(100);
		constexpr auto Replicator_Count = 5;
        constexpr auto Download_Size = 100;
        constexpr Amount Storage_Units(Download_Size);
        constexpr Height Current_Height(15);

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			auto pluginConfig = config::StorageConfiguration::Uninitialized();
			pluginConfig.ShardSize = 20;
			config.Network.SetPluginConfiguration(pluginConfig);
			return (config.ToConst());
		}

        struct CacheValues {
            public:
                explicit CacheValues()
                    : BcDriveEntry(Key())
					, ReplicatorEntries()
                    , ExpectedDownloadChannelEntry(Hash256())
                {}
            
            public:
				state::BcDriveEntry BcDriveEntry;
				std::vector<state::ReplicatorEntry> ReplicatorEntries;
				state::DownloadChannelEntry ExpectedDownloadChannelEntry;
        };

        state::DownloadChannelEntry CreateExpectedDownloadChannelEntry(const Hash256& id, const state::BcDriveEntry& driveEntry) {
            state::DownloadChannelEntry entry(id);
            entry.setConsumer(Consumer);
			entry.setDrive(driveEntry.key());
			entry.setDownloadSize(Download_Size);
			entry.setDownloadApprovalCountLeft(1);
			for (const auto& key : driveEntry.replicators()) {
				entry.shardReplicators().insert(key);
				entry.cumulativePayments().emplace(key, 0);
			}
			entry.listOfPublicKeys().push_back(Consumer);

			return entry;
        }

		std::vector<state::ReplicatorEntry> CreateReplicatorEntries(const uint16_t& count) {
			std::vector<state::ReplicatorEntry> entries;
			entries.reserve(count);
			for (auto i = 0u; i < count; ++i)
				entries.push_back(test::CreateReplicatorEntry(test::GenerateRandomByteArray<Key>()));

			return entries;
		}

        state::BcDriveEntry CreateBcDriveEntry(
				const Key& driveKey,
				const std::vector<state::ReplicatorEntry>& replicatorEntries) {
            state::BcDriveEntry driveEntry(driveKey);
			for (const auto& replicatorEntry : replicatorEntries)
				driveEntry.replicators().emplace(replicatorEntry.key());

            return driveEntry;
        }

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
			ObserverTestContext context(mode, currentHeight, CreateConfig());
			Notification notification(
                values.ExpectedDownloadChannelEntry.id(),
                values.ExpectedDownloadChannelEntry.consumer(),
				values.ExpectedDownloadChannelEntry.drive(),
                Download_Size,
				0u,
				nullptr);
            auto pObserver = CreateDownloadChannelObserver();
			auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			auto& driveCache = context.cache().sub<cache::BcDriveCache>();
			auto& downloadChannelCache = context.cache().sub<cache::DownloadChannelCache>();

            // Populate cache.
			for (auto& entry : values.ReplicatorEntries)
				replicatorCache.insert(entry);
            driveCache.insert(values.BcDriveEntry);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto downloadChannelIter = downloadChannelCache.find(values.ExpectedDownloadChannelEntry.id());
            const auto& actualDownloadChannelEntry = downloadChannelIter.get();
            test::AssertEqualDownloadChannelData(values.ExpectedDownloadChannelEntry, actualDownloadChannelEntry);

            auto driveIter = driveCache.find(values.BcDriveEntry.key());
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(values.BcDriveEntry, actualEntry);
        }
    }

    TEST(TEST_CLASS, DownloadChannel_Commit) {
        // Arrange:
        CacheValues values;
        auto driveKey = test::GenerateRandomByteArray<Key>();
        auto activeDownloadId = test::GenerateRandomByteArray<Hash256>();
		values.ReplicatorEntries = CreateReplicatorEntries(Replicator_Count);
        values.BcDriveEntry = CreateBcDriveEntry(driveKey, values.ReplicatorEntries);
        values.ExpectedDownloadChannelEntry = CreateExpectedDownloadChannelEntry(activeDownloadId, values.BcDriveEntry);

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