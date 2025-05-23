/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/other/mocks/MockStorageState.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS PeriodicStoragePaymentObserverTests

	const std::unique_ptr<observers::LiquidityProviderExchangeObserver>  Liquidity_Provider = std::make_unique<test::LiquidityProviderExchangeObserverImpl>();

    DEFINE_COMMON_OBSERVER_TESTS(PeriodicStoragePayment, Liquidity_Provider, {}, nullptr)

	const auto billingPeriodMilliseconds = 20000;

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::BlockNotification<1>;

        constexpr Height Current_Height(20);
		const auto Zero_Key = Key();
		const auto Owner_Key = test::GenerateRandomByteArray<Key>();
        constexpr auto Drive_Size = 100;
        constexpr auto Num_Replicators = 2;
		constexpr auto Modification_Size = 10;
		constexpr Amount Drive_Balance(200);
		constexpr auto Currency_Mosaic_Id = MosaicId(1234);
		constexpr auto Storage_Mosaic_Id = MosaicId(2341);
		constexpr auto Streaming_Mosaic_Id = MosaicId(4321);
		constexpr Amount Storage_Lock_Amount(Drive_Size * Num_Replicators);

		constexpr Amount Expected_Replicator_Balance( Modification_Size * (2*Num_Replicators - 1) / Num_Replicators );
		constexpr Amount Expected_Owner_Balance = Drive_Balance - Amount(Num_Replicators * Expected_Replicator_Balance.unwrap());


		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			config.Immutable.StorageMosaicId = Storage_Mosaic_Id;
			config.Immutable.StreamingMosaicId = Streaming_Mosaic_Id;

			auto storageConfig = config::StorageConfiguration::Uninitialized();
			storageConfig.StorageBillingPeriod = utils::TimeSpan::FromMilliseconds(billingPeriodMilliseconds);
			storageConfig.Enabled = true;
			storageConfig.EnableCacheImprovement = true;

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
			CacheValues(const std::vector<state::BcDriveEntry>& initialBcDriveEntries,
						const std::vector<Key>& expectedBcDriveKeys,
						const std::vector<state::ReplicatorEntry>& initialReplicatorEntries,
						const std::vector<state::ReplicatorEntry>& expectedReplicatorEntries,
						const Timestamp& notificationTime)
				: InitialBcDriveEntries(initialBcDriveEntries)
				, ExpectedBcDriveKeys(expectedBcDriveKeys)
				, InitialReplicatorEntries(initialReplicatorEntries)
				, ExpectedReplicatorEntries(expectedReplicatorEntries)
				, NotificationTime(notificationTime)
			{}

		public:
			std::vector<state::BcDriveEntry> InitialBcDriveEntries;
			std::vector<Key> ExpectedBcDriveKeys;
			std::vector<state::ReplicatorEntry> InitialReplicatorEntries;
			std::vector<state::ReplicatorEntry> ExpectedReplicatorEntries;
			Timestamp NotificationTime;
		};

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height, CreateConfig());
            Notification notification({ { 1 } }, { { 1 } }, values.NotificationTime, Difficulty(0), 0, 0);
			auto pStorageState = std::make_shared<mocks::MockStorageState>();
            auto pObserver = CreatePeriodicStoragePaymentObserver(Liquidity_Provider, {}, pStorageState);
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();
			auto& queueCache = context.cache().sub<cache::QueueCache>();

            // Populate cache.
			test::AddAccountState(accountStateCache, Zero_Key, Current_Height, {{Storage_Mosaic_Id, Storage_Lock_Amount}});

			for (const auto& entry: values.InitialBcDriveEntries) {
				bcDriveCache.insert(entry);
				test::AddAccountState(accountStateCache, entry.key(), Current_Height, {{Streaming_Mosaic_Id, Drive_Balance}});
				test::AddAccountState(accountStateCache, entry.owner(), Current_Height);
			}

			for (const auto& entry : values.InitialReplicatorEntries) {
				replicatorCache.insert(entry);
				test::AddAccountState(accountStateCache, entry.key(), Current_Height);
			}

			if (!values.InitialBcDriveEntries.empty()) {
				state::QueueEntry queueEntry(state::DrivePaymentQueueKey);
				queueEntry.setFirst(values.InitialBcDriveEntries[0].key());
				queueEntry.setLast(values.InitialBcDriveEntries[values.InitialBcDriveEntries.size() - 1].key());
				queueEntry.setSize(values.InitialBcDriveEntries.size());
				queueCache.insert(queueEntry);
			}

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto& queueCacheEntry = queueCache.find(state::DrivePaymentQueueKey).get();
            auto driveKey = queueCacheEntry.getFirst();
			auto previousKey = Key();

			if (values.ExpectedBcDriveKeys.empty()) {
				EXPECT_EQ(queueCacheEntry.getFirst(), Key());
				EXPECT_EQ(queueCacheEntry.getLast(), Key());
				return;
			}

			EXPECT_EQ(queueCacheEntry.getLast(), values.ExpectedBcDriveKeys[values.ExpectedBcDriveKeys.size() - 1]);

			for (const auto& key: values.ExpectedBcDriveKeys) {
				EXPECT_EQ(driveKey, key);
				EXPECT_EQ(bcDriveCache.find(driveKey).get().getQueuePrevious(), previousKey);
				previousKey = driveKey;
				driveKey = bcDriveCache.find(driveKey).get().getQueueNext();
			}

			EXPECT_EQ(driveKey, Key());

			for (const auto& expectedReplicatorEntry: values.ExpectedReplicatorEntries) {
				auto actualReplicatorEntryIter = replicatorCache.find(expectedReplicatorEntry.key());
				auto& actualReplicatorEntry = actualReplicatorEntryIter.get();

				EXPECT_EQ(expectedReplicatorEntry.drives().size(), actualReplicatorEntry.drives().size());

				for (const auto& expectedDriveKey : expectedReplicatorEntry.drives()) {
					auto actualDrivesBegin = actualReplicatorEntry.drives().begin();
					auto actualDrivesEnd = actualReplicatorEntry.drives().end();

					EXPECT_TRUE(std::any_of(actualDrivesBegin, actualDrivesEnd, [expectedDriveKey]
							   (const auto& actualDriveKey) { return actualDriveKey.first == expectedDriveKey.first; }));
				}
			}
        }
    }

    TEST(TEST_CLASS, PeriodicStoragePayment_RemoveDrive) {
    	// Arrange:

    	Timestamp firstTimestamp(10000);
    	Timestamp notificationTimestamp = Timestamp(firstTimestamp.unwrap() + billingPeriodMilliseconds);
    	Timestamp secondTimestamp = firstTimestamp;
    	Timestamp thirdTimestamp = notificationTimestamp;

    	Key firstDriveKey =  { { 1 } };
    	Key secondDriveKey = { { 2 } };
    	Key thirdDriveKey = { { 3 } };

    	state::BcDriveEntry firstDriveEntry(firstDriveKey);
		firstDriveEntry.setSize(0);
		firstDriveEntry.setReplicatorCount(Num_Replicators);
		firstDriveEntry.setQueueNext(secondDriveKey);
		firstDriveEntry.setLastPayment(firstTimestamp);

    	state::BcDriveEntry secondDriveEntry(secondDriveKey);
		secondDriveEntry.setSize(1);
		secondDriveEntry.setReplicatorCount(Num_Replicators);
		secondDriveEntry.setQueuePrevious(firstDriveKey);
		secondDriveEntry.setQueueNext(thirdDriveKey);
		secondDriveEntry.setLastPayment(secondTimestamp);

    	state::BcDriveEntry thirdDriveEntry(thirdDriveKey);
		thirdDriveEntry.setSize(0);
		thirdDriveEntry.setReplicatorCount(Num_Replicators);
		thirdDriveEntry.setQueuePrevious(secondDriveKey);
		thirdDriveEntry.setLastPayment(thirdTimestamp);

		state::ReplicatorEntry firstReplicatorEntry({ { 4 } });
		firstReplicatorEntry.drives() = { { firstDriveEntry.key(), {} },
										  { secondDriveEntry.key(), {} },
										  { thirdDriveEntry.key(), {} } };

		state::ReplicatorEntry secondReplicatorEntry({ { 5 } });
		secondReplicatorEntry.drives() = { { firstDriveEntry.key(), {} },
										   { secondDriveEntry.key(), {} },
										   { thirdDriveEntry.key(), {} } };

		std::vector<state::ReplicatorEntry> initialReplicatorEntries = { firstReplicatorEntry, secondReplicatorEntry };

		firstReplicatorEntry.drives().erase(secondDriveKey);
		secondReplicatorEntry.drives().erase(secondDriveKey);

		std::vector<state::ReplicatorEntry> expectedReplicatorEntries = { firstReplicatorEntry, secondReplicatorEntry };

		firstDriveEntry.replicators() = { firstReplicatorEntry.key(), secondReplicatorEntry.key() };
		secondDriveEntry.replicators() = { firstReplicatorEntry.key(), secondReplicatorEntry.key() };
		thirdDriveEntry.replicators() = { firstReplicatorEntry.key(), secondReplicatorEntry.key() };

		std::vector<state::BcDriveEntry> initialBcDriveEntries = { firstDriveEntry, secondDriveEntry, thirdDriveEntry };
		std::vector<Key> expectedKeys = { thirdDriveKey, firstDriveKey };

    	CacheValues values(initialBcDriveEntries, expectedKeys, initialReplicatorEntries, expectedReplicatorEntries, notificationTimestamp);

    	// Assert
    	RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, PeriodicStoragePayment_SingleDrive) {
    	// Arrange:

    	Timestamp firstTimestamp(10000);
    	auto notificationTimestamp = Timestamp(firstTimestamp.unwrap() + billingPeriodMilliseconds);

    	Key firstKey =  { { 1 } };
    	Key secondKey = { { 2 } };
    	Key thirdKey = { { 3 } };

    	state::BcDriveEntry firstEntry(firstKey);
    	firstEntry.setSize(0);
    	firstEntry.setLastPayment(firstTimestamp);

    	std::vector<state::BcDriveEntry> initialEntries = {firstEntry};
    	std::vector<Key> expectedKeys = {firstKey};

    	CacheValues values(initialEntries, expectedKeys, {}, {}, notificationTimestamp);

    	// Assert
    	RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, PeriodicStoragePayment_ContinueDrive) {
    	// Arrange:

    	Timestamp firstTimestamp(10000);
    	auto notificationTimestamp = Timestamp(firstTimestamp.unwrap() + billingPeriodMilliseconds);
    	Timestamp secondTimestamp = firstTimestamp;
    	Timestamp thirdTimestamp = notificationTimestamp;

    	Key firstKey =  { { 1 } };
    	Key secondKey = { { 2 } };
    	Key thirdKey = { { 3 } };

    	state::BcDriveEntry firstEntry(firstKey);
    	firstEntry.setSize(0);
		firstEntry.setQueueNext(secondKey);
    	firstEntry.setLastPayment(firstTimestamp);

    	state::BcDriveEntry secondEntry(secondKey);
    	secondEntry.setSize(0);
		secondEntry.setQueuePrevious(firstKey);
		secondEntry.setQueueNext(thirdKey);
    	secondEntry.setLastPayment(secondTimestamp);

    	state::BcDriveEntry thirdEntry(thirdKey);
    	thirdEntry.setSize(0);
		thirdEntry.setQueuePrevious(secondKey);
    	thirdEntry.setLastPayment(thirdTimestamp);

    	std::vector<state::BcDriveEntry> initialEntries = {firstEntry, secondEntry, thirdEntry};
    	std::vector<Key> expectedKeys = {thirdKey, firstKey, secondKey};

    	CacheValues values(initialEntries, expectedKeys, {}, {}, notificationTimestamp);

    	// Assert
    	RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, PeriodicStoragePayment_NoDrives) {
    	// Arrange:
    	auto notificationTimestamp = Timestamp(billingPeriodMilliseconds);

    	std::vector<state::BcDriveEntry> initialEntries = {};
    	std::vector<Key> expectedKeys = {};

    	CacheValues values(initialEntries, expectedKeys, {}, {}, notificationTimestamp);

    	// Assert
    	RunTest(NotifyMode::Commit, values, Current_Height);
    }

    TEST(TEST_CLASS, PeriodicStoragePayment_Rollback) {
        // Arrange:
        CacheValues values({}, {}, {}, {}, Timestamp());

        // Assert
		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height), catapult_runtime_error);
    }
}}