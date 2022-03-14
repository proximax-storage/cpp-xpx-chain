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

	using DrivePriority = std::pair<Key, double>;
	using DriveQueue = std::priority_queue<DrivePriority, std::vector<DrivePriority>, utils::DriveQueueComparator>;

    DEFINE_COMMON_OBSERVER_TESTS(DriveClosure, std::make_shared<DriveQueue>())

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::DriveClosureNotification<1>;

        constexpr Height Current_Height(20);
		const auto Drive_Queue = std::make_shared<DriveQueue>();
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
			CacheValues()
				: InitialBcDriveEntry(Key())
				, ExpectedBcDriveEntry(Key())
			{}

		public:
			state::BcDriveEntry InitialBcDriveEntry;
			state::BcDriveEntry ExpectedBcDriveEntry;
			std::vector<state::ReplicatorEntry> InitialReplicatorEntries;
			std::vector<state::ReplicatorEntry> ExpectedReplicatorEntries;\
		};

        void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, Current_Height, CreateConfig());
            Notification notification(Hash256(), values.InitialBcDriveEntry.key(), test::GenerateRandomByteArray<Key>());
            auto pObserver = CreateDriveClosureObserver(Drive_Queue);
            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
        	auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();

            // Populate cache.
            bcDriveCache.insert(values.InitialBcDriveEntry);
			test::AddAccountState(accountStateCache, values.InitialBcDriveEntry.key(), Current_Height, {{Streaming_Mosaic_Id, Drive_Balance}});
			test::AddAccountState(accountStateCache, values.InitialBcDriveEntry.owner(), Current_Height);
            for (const auto& entry : values.InitialReplicatorEntries) {
				replicatorCache.insert(entry);
				test::AddAccountState(accountStateCache, entry.key(), Current_Height);
			}

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
			EXPECT_FALSE(bcDriveCache.find(values.ExpectedBcDriveEntry.key()).tryGet());

            for (const auto& entry : values.ExpectedReplicatorEntries) {
				auto replicatorIter = replicatorCache.find(entry.key());
				const auto &actualEntry = replicatorIter.get();
				test::AssertEqualReplicatorData(entry, actualEntry);
				EXPECT_EQ(accountStateCache.find(entry.key()).get().Balances.get(Currency_Mosaic_Id), Expected_Replicator_Balance);
			}
			EXPECT_EQ(accountStateCache.find(Owner_Key).get().Balances.get(Currency_Mosaic_Id), Expected_Owner_Balance);
        }
    }

    TEST(TEST_CLASS, DriveClosure_Commit) {
        // Arrange:
        CacheValues values;
        auto driveKey = test::GenerateRandomByteArray<Key>();
        utils::SortedKeySet replicatorKeys;
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