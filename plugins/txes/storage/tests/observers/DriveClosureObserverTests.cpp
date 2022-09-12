/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/observers/Observers.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS DriveClosureObserverTests

	const auto Liquidity_Provider = std::make_shared<test::LiquidityProviderExchangeObserverImpl>();

    DEFINE_COMMON_OBSERVER_TESTS(DriveClosure, *Liquidity_Provider)

    const auto billingPeriodSeconds = 20000;

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::DriveClosureNotification<1>;

        constexpr Height Current_Height(20);
		const auto Zero_Key = Key();
		const auto Owner_Key = test::GenerateRandomByteArray<Key>();
        constexpr auto Drive_Size = 100;
        constexpr auto Num_Replicators = 10;
		constexpr auto Modification_Size = 10;
		constexpr Amount Drive_Balance(200
									   + 2 * Num_Replicators * Drive_Size);	// Streaming deposit
		constexpr Amount Storage_Lock_Amount(Drive_Size * Num_Replicators);
		constexpr auto Currency_Mosaic_Id = MosaicId(1);
		constexpr auto Storage_Mosaic_Id = MosaicId(2);
		constexpr auto Streaming_Mosaic_Id = MosaicId(3);

		constexpr Amount Expected_Replicator_Balance( Modification_Size * (2*Num_Replicators - 1) / Num_Replicators );
		constexpr Amount Expected_Owner_Balance = Drive_Balance - Amount(Num_Replicators * Expected_Replicator_Balance.unwrap());


		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			config.Immutable.StorageMosaicId = Storage_Mosaic_Id;
			config.Immutable.StreamingMosaicId = Streaming_Mosaic_Id;

			auto storageConfig = config::StorageConfiguration::Uninitialized();
			storageConfig.StorageBillingPeriod = utils::TimeSpan::FromMilliseconds(billingPeriodSeconds);

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

		state::ReplicatorEntry CreateInitialReplicatorEntry(const std::vector<Key>& driveKeys, const Key& replicatorKey){
			state::ReplicatorEntry entry(replicatorKey);
			for (const auto& driveKey: driveKeys) {
				entry.drives().emplace(driveKey, state::DriveInfo());
			}

			return entry;
		}

        struct CacheValues {
			std::vector<state::BcDriveEntry> InitialBcDriveEntries;
			std::vector<state::BcDriveEntry> ExpectedBcDriveEntries;
			std::vector<state::ReplicatorEntry> InitialReplicatorEntries;
			std::vector<state::ReplicatorEntry> ExpectedReplicatorEntries;
		};

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight, const Key& driveToRemove) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height, CreateConfig());
            Notification notification(Hash256(), driveToRemove, test::GenerateRandomByteArray<Key>());
            auto pObserver = CreateDriveClosureObserver(*Liquidity_Provider);
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();
			auto& queueStateCache = context.cache().sub<cache::QueueCache>();

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
				queueEntry.setFirst(values.InitialBcDriveEntries.front().key());
				queueEntry.setLast(values.InitialBcDriveEntries.back().key());
				queueStateCache.insert(queueEntry);
			}

			// Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			EXPECT_FALSE(bcDriveCache.find(driveToRemove).tryGet());

//          for (const auto& entry : values.ExpectedReplicatorEntries) {
//				auto replicatorIter = replicatorCache.find(entry.key());
//				const auto &actualEntry = replicatorIter.get();
//				test::AssertEqualReplicatorData(entry, actualEntry);
//				EXPECT_EQ(accountStateCache.find(entry.key()).get().Balances.get(Currency_Mosaic_Id), Expected_Replicator_Balance);
//			}
//			EXPECT_EQ(accountStateCache.find(Owner_Key).get().Balances.get(Currency_Mosaic_Id), Expected_Owner_Balance);

            auto& queueCacheEntry = queueStateCache.find(state::DrivePaymentQueueKey).get();
            auto driveKey = queueCacheEntry.getFirst();
            auto previousKey = Key();

            EXPECT_EQ(queueCacheEntry.getLast(), values.ExpectedBcDriveEntries.back().key());
            for (const auto& entry: values.ExpectedBcDriveEntries) {
            	EXPECT_EQ(driveKey, entry.key());
            	EXPECT_EQ(bcDriveCache.find(driveKey).get().getQueuePrevious(), previousKey);
            	previousKey = driveKey;
            	driveKey = bcDriveCache.find(driveKey).get().getQueueNext();
            }
            EXPECT_EQ(driveKey, Key());
		}
    }

//    TEST(TEST_CLASS, DriveClosure_RemoveFirstDrive) {
//    	// Arrange:
//    	CacheValues values;
//
//    	std::vector<Key> driveKeys;
//    	for (int i = 0; i < 3; i++) {
//    		driveKeys.push_back(test::GenerateRandomByteArray<Key>());
//    	}
//
//		auto keyToRemove = driveKeys[0];
//    	std::vector<Key> expectedDriveKeys = driveKeys;
//    	expectedDriveKeys.erase(expectedDriveKeys.begin());
//
//    	utils::SortedKeySet replicatorKeys;
//    	for (auto i = 0u; i < Num_Replicators; ++i) {
//    		auto replicatorKey = test::GenerateRandomByteArray<Key>();
//    		replicatorKeys.emplace(replicatorKey);
//    		values.InitialReplicatorEntries.push_back(CreateInitialReplicatorEntry(driveKeys, replicatorKey));
//    		values.ExpectedReplicatorEntries.push_back(CreateInitialReplicatorEntry(expectedDriveKeys, replicatorKey));
//    	}
//
//    	for (const auto& driveKey: driveKeys) {
//    		values.InitialBcDriveEntries.push_back(CreateInitialBcDriveEntry(driveKey, replicatorKeys));
//    	}
//
//    	for (int i = 0; i < values.InitialBcDriveEntries.size() - 1; i++) {
//			values.InitialBcDriveEntries[i].setQueueNext(values.InitialBcDriveEntries[i + 1].key());
//    	}
//
//    	for (int i = 1; i < values.InitialBcDriveEntries.size(); i++) {
//			values.InitialBcDriveEntries[i].setQueuePrevious(values.InitialBcDriveEntries[i - 1].key());
//    	}
//
//    	for (const auto& driveKey: expectedDriveKeys) {
//    		values.ExpectedBcDriveEntries.push_back(CreateInitialBcDriveEntry(driveKey, replicatorKeys));
//    	}
//
//    	// Assert
//    	RunTest(NotifyMode::Commit, values, Current_Height, keyToRemove);
//    }
//
//    TEST(TEST_CLASS, DriveClosure_RemoveMiddleDrive) {
//        // Arrange:
//        CacheValues values;
//
//        std::vector<Key> driveKeys;
//        for (int i = 0; i < 3; i++) {
//        	driveKeys.push_back(test::GenerateRandomByteArray<Key>());
//		}
//
//        auto keyToRemove = driveKeys[1];
//		std::vector<Key> expectedDriveKeys = driveKeys;
//		expectedDriveKeys.erase(expectedDriveKeys.begin() + 1);
//
//        utils::SortedKeySet replicatorKeys;
//        for (auto i = 0u; i < Num_Replicators; ++i) {
//        	auto replicatorKey = test::GenerateRandomByteArray<Key>();
//			replicatorKeys.emplace(replicatorKey);
//			values.InitialReplicatorEntries.push_back(CreateInitialReplicatorEntry(driveKeys, replicatorKey));
//			values.ExpectedReplicatorEntries.push_back(CreateInitialReplicatorEntry(expectedDriveKeys, replicatorKey));
//		}
//
//		for (const auto& driveKey: driveKeys) {
//			values.InitialBcDriveEntries.push_back(CreateInitialBcDriveEntry(driveKey, replicatorKeys));
//		}
//
//		for (int i = 0; i < values.InitialBcDriveEntries.size() - 1; i++) {
//			values.InitialBcDriveEntries[i].setQueueNext(values.InitialBcDriveEntries[i + 1].key());
//		}
//
//		for (int i = 1; i < values.InitialBcDriveEntries.size(); i++) {
//			values.InitialBcDriveEntries[i].setQueuePrevious(values.InitialBcDriveEntries[i - 1].key());
//		}
//
//		for (const auto& driveKey: expectedDriveKeys) {
//			values.ExpectedBcDriveEntries.push_back(CreateInitialBcDriveEntry(driveKey, replicatorKeys));
//		}
//
//        // Assert
//        RunTest(NotifyMode::Commit, values, Current_Height, keyToRemove);
//    }
//
//    TEST(TEST_CLASS, DriveClosure_RemoveLastDrive) {
//    	// Arrange:
//    	CacheValues values;
//
//    	std::vector<Key> driveKeys;
//    	for (int i = 0; i < 3; i++) {
//    		driveKeys.push_back(test::GenerateRandomByteArray<Key>());
//    	}
//
//    	auto keyToRemove = driveKeys[2];
//    	std::vector<Key> expectedDriveKeys = driveKeys;
//    	expectedDriveKeys.erase(expectedDriveKeys.begin() + 2);
//
//    	utils::SortedKeySet replicatorKeys;
//    	for (auto i = 0u; i < Num_Replicators; ++i) {
//    		auto replicatorKey = test::GenerateRandomByteArray<Key>();
//    		replicatorKeys.emplace(replicatorKey);
//    		values.InitialReplicatorEntries.push_back(CreateInitialReplicatorEntry(driveKeys, replicatorKey));
//    		values.ExpectedReplicatorEntries.push_back(CreateInitialReplicatorEntry(expectedDriveKeys, replicatorKey));
//    	}
//
//    	for (const auto& driveKey: driveKeys) {
//    		values.InitialBcDriveEntries.push_back(CreateInitialBcDriveEntry(driveKey, replicatorKeys));
//    	}
//
//    	for (int i = 0; i < values.InitialBcDriveEntries.size() - 1; i++) {
//			values.InitialBcDriveEntries[i].setQueueNext(values.InitialBcDriveEntries[i + 1].key());
//    	}
//
//    	for (int i = 1; i < values.InitialBcDriveEntries.size(); i++) {
//			values.InitialBcDriveEntries[i].setQueuePrevious(values.InitialBcDriveEntries[i - 1].key());
//    	}
//
//    	for (const auto& driveKey: expectedDriveKeys) {
//    		values.ExpectedBcDriveEntries.push_back(CreateInitialBcDriveEntry(driveKey, replicatorKeys));
//    	}
//
//    	// Assert
//    	RunTest(NotifyMode::Commit, values, Current_Height, keyToRemove);
//    }
//
//    TEST(TEST_CLASS, DriveClosure_Rollback) {
//        // Arrange:
//        CacheValues values;
//
//        // Assert
//		EXPECT_THROW(RunTest(NotifyMode::Rollback, values, Current_Height, Key()), catapult_runtime_error);
//    }
}}