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

#define TEST_CLASS PeriodicDownloadChannelPaymentObserverTests

	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

	DEFINE_COMMON_OBSERVER_TESTS(PeriodicDownloadChannelPayment,)

	const auto billingPeriodSeconds = 20000;
	const auto Drive_Queue = std::make_shared<DriveQueue>();

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::BlockNotification<2>;

        constexpr Height Current_Height(20);
		const auto Owner_Key = test::GenerateRandomByteArray<Key>();
        constexpr auto Drive_Size = 100;
        constexpr auto Num_Replicators = 10;
		constexpr auto Modification_Size = 10;
		constexpr Amount Drive_Balance(200);
		constexpr auto Currency_Mosaic_Id = MosaicId(1234);
		constexpr auto Streaming_Mosaic_Id = MosaicId(4321);

		constexpr Amount Expected_Replicator_Balance( Modification_Size * (2*Num_Replicators - 1) / Num_Replicators );
		constexpr Amount Expected_Owner_Balance = Drive_Balance - Amount(Num_Replicators * Expected_Replicator_Balance.unwrap());


		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			config.Immutable.StreamingMosaicId = Streaming_Mosaic_Id;

			auto storageConfig = config::StorageConfiguration::Uninitialized();
			storageConfig.DownloadBillingPeriod = utils::TimeSpan::FromMilliseconds(billingPeriodSeconds);

			config.Network.SetPluginConfiguration(storageConfig);

			return config.ToConst();
		}

        state::BcDriveEntry CreateInitialBcDriveEntry(const Key& driveKey, const utils::SortedKeySet& replicatorKeys){
            state::BcDriveEntry entry(driveKey);
			entry.setOwner(Owner_Key);
            entry.setSize(Drive_Size);
            entry.setReplicatorCount(Num_Replicators);
			entry.replicators() = replicatorKeys;
			entry.activeDataModifications().emplace_back(state::ActiveDataModification {
					test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Key>(),
					test::GenerateRandomByteArray<Hash256>(), Modification_Size
			});

            return entry;
        }

		state::ReplicatorEntry CreateInitialReplicatorEntry(const Key& driveKey, const Key& replicatorKey){
			state::ReplicatorEntry entry(replicatorKey);
			entry.drives().emplace(driveKey, state::DriveInfo());

			return entry;
		}

        state::ReplicatorEntry CreateExpectedReplicatorEntry(const Key& replicatorKey){
            state::ReplicatorEntry entry(replicatorKey);

            return entry;
        }

        struct CacheValues {
		public:
			CacheValues(const std::vector<state::DownloadChannelEntry>& initialEntries,
						const std::vector<Hash256>& expectedKeys,
						const Timestamp& notificationTime)
				: InitialEntries(initialEntries)
				, ExpectedKeys(expectedKeys)
				, NotificationTime(notificationTime)
			{}

		public:
			std::vector<state::DownloadChannelEntry> InitialEntries;
			std::vector<Hash256> ExpectedKeys;
			std::vector<state::ReplicatorEntry> InitialReplicatorEntries;
			std::vector<state::ReplicatorEntry> ExpectedReplicatorEntries;
			Timestamp NotificationTime;
		};

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height, CreateConfig());
            Notification notification({ { 1 } }, values.NotificationTime);
            auto pObserver = CreatePeriodicDownloadChannelPaymentObserver();
            auto& downloadCache = context.cache().sub<cache::DownloadChannelCache>();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();
			auto& queueCache = context.cache().sub<cache::QueueCache>();

            // Populate cache.
			if (!values.InitialEntries.empty()) {
				state::QueueEntry queueEntry(state::DownloadChannelPaymentQueueKey);
				queueEntry.setFirst(values.InitialEntries[0].id().array());
				queueEntry.setLast(values.InitialEntries[values.InitialEntries.size() - 1].id().array());
				queueCache.insert(queueEntry);
			}

			for (const auto& entry: values.InitialEntries) {
				downloadCache.insert(entry);
			}

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto& queueCacheEntry = queueCache.find(state::DownloadChannelPaymentQueueKey).get();
            Hash256 channelId = queueCacheEntry.getFirst().array();
			auto previousKey = Hash256();

			EXPECT_EQ(queueCacheEntry.getLast().array(), values.ExpectedKeys.back().array());
			for (const auto& id: values.ExpectedKeys) {
				EXPECT_EQ(channelId, id);
				EXPECT_EQ(downloadCache.find(channelId).get().getQueuePrevious().array(), previousKey.array());
				previousKey = id;
				channelId = downloadCache.find(channelId).get().getQueueNext().array();
			}
			EXPECT_EQ(channelId, Hash256());
        }
    }

    TEST(TEST_CLASS, PeriodicDownloadPayment_RemoveChannel) {
    	// Arrange:

    	Timestamp firstTimestamp(10000);
    	Timestamp notificationTimestamp = Timestamp(firstTimestamp.unwrap() + billingPeriodSeconds);
    	Timestamp secondTimestamp = firstTimestamp;
    	Timestamp thirdTimestamp = notificationTimestamp;

    	Hash256 firstKey =  { { 1 } };
    	Hash256 secondKey = { { 2 } };
    	Hash256 thirdKey = { { 3 } };

    	state::DownloadChannelEntry firstEntry(firstKey);
    	firstEntry.setQueueNext(secondKey.array());
    	firstEntry.setDownloadApprovalCountLeft(2);
    	firstEntry.setLastDownloadApprovalInitiated(firstTimestamp);

    	state::DownloadChannelEntry secondEntry(secondKey);
    	secondEntry.setQueuePrevious(firstKey.array());
    	secondEntry.setQueueNext(thirdKey.array());
    	secondEntry.setDownloadApprovalCountLeft(1);
    	secondEntry.setLastDownloadApprovalInitiated(secondTimestamp);

    	state::DownloadChannelEntry thirdEntry(thirdKey);
    	thirdEntry.setQueuePrevious(secondKey.array());
    	thirdEntry.setDownloadApprovalCountLeft(2);
    	thirdEntry.setLastDownloadApprovalInitiated(thirdTimestamp);

    	std::vector<state::DownloadChannelEntry> initialEntries = {firstEntry, secondEntry, thirdEntry};
    	std::vector<Hash256> expectedKeys = {thirdKey, firstKey};

    	CacheValues values(initialEntries, expectedKeys, notificationTimestamp);

    	// Assert
    	RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, PeriodicDownloadPayment_ContinueChannels) {
    	// Arrange:

    	Timestamp firstTimestamp(10000);
    	Timestamp notificationTimestamp = Timestamp(firstTimestamp.unwrap() + billingPeriodSeconds);
    	Timestamp secondTimestamp = firstTimestamp;
    	Timestamp thirdTimestamp = notificationTimestamp;

    	Hash256 firstKey =  { { 1 } };
    	Hash256 secondKey = { { 2 } };
    	Hash256 thirdKey = { { 3 } };

    	state::DownloadChannelEntry firstEntry(firstKey);
    	firstEntry.setQueueNext(secondKey.array());
    	firstEntry.setDownloadApprovalCountLeft(2);
    	firstEntry.setLastDownloadApprovalInitiated(firstTimestamp);

    	state::DownloadChannelEntry secondEntry(secondKey);
    	secondEntry.setQueuePrevious(firstKey.array());
    	secondEntry.setQueueNext(thirdKey.array());
    	secondEntry.setDownloadApprovalCountLeft(2);
    	secondEntry.setLastDownloadApprovalInitiated(secondTimestamp);

    	state::DownloadChannelEntry thirdEntry(thirdKey);
    	thirdEntry.setQueuePrevious(secondKey.array());
    	secondEntry.setDownloadApprovalCountLeft(2);
    	thirdEntry.setLastDownloadApprovalInitiated(thirdTimestamp);

    	std::vector<state::DownloadChannelEntry> initialEntries = {firstEntry, secondEntry, thirdEntry};
    	std::vector<Hash256> expectedKeys = {thirdKey, firstKey, secondKey};

    	CacheValues values(initialEntries, expectedKeys, notificationTimestamp);

    	// Assert
    	RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, PeriodicStoragePayment_Rollback) {
        // Arrange:
        CacheValues values({}, {}, Timestamp());

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}