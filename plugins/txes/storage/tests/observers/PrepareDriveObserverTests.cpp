/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
#include "src/observers/Observers.h"
#include "src/utils/AVLTree.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS PrepareDriveObserverTests

	DEFINE_COMMON_OBSERVER_TESTS(PrepareDrive,)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::PrepareDriveNotification<1>;
		using ReplicatorWithAmounts = std::tuple<state::ReplicatorEntry, Amount, Amount>;	// Replicator entry, SO amount, SM amount

        const Key Drive_Key = test::GenerateRandomByteArray<Key>();
        const Key Owner = test::GenerateRandomByteArray<Key>();
        constexpr auto Drive_Size = 50;
        constexpr auto Replicator_Count = 3;
		constexpr auto Unacceptable_Replicator_Count = 1;	// Number of replicators that don't have enough mosaics
        constexpr Height Current_Height(20);
		constexpr auto Min_Replicator_Count = 2;
		constexpr auto Storage_Mosaic_Id = MosaicId(1234);
		constexpr auto Streaming_Mosaic_Id = MosaicId(4321);

		constexpr auto Total_Replicator_Count = Replicator_Count + Unacceptable_Replicator_Count;

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.StorageMosaicId = Storage_Mosaic_Id;
			config.Immutable.StreamingMosaicId = Streaming_Mosaic_Id;

			auto pluginConfig = config::StorageConfiguration::Uninitialized();
			pluginConfig.MinReplicatorCount = Min_Replicator_Count;
			config.Network.SetPluginConfiguration(pluginConfig);

			return config.ToConst();
		}

		utils::AVLTreeAdapter<std::pair<Amount, Key>> CreateAvlTreeAdapter(const observers::ObserverContext& context) {
			auto& queueCache = context.Cache.sub<cache::QueueCache>();
			auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
			auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();

			auto keyExtractor = [=, &accountStateCache](const Key& key) {
			  	return std::make_pair(accountStateCache.find(key).get().Balances.get(Storage_Mosaic_Id), key);
			};
			auto nodeExtractor = [&replicatorCache](const Key& key) -> state::AVLTreeNode {
			  	return replicatorCache.find(key).get().replicatorsSetNode();
			};
			auto nodeSaver = [&replicatorCache](const Key& key, const state::AVLTreeNode& node) {
			  	replicatorCache.find(key).get().replicatorsSetNode() = node;
			};

			return utils::AVLTreeAdapter<std::pair<Amount, Key>> (
					queueCache,
					state::ReplicatorsSetTree,
					keyExtractor,
					nodeExtractor,
					nodeSaver
			);
		}
        
        state::BcDriveEntry CreateBcDriveEntry() {
            state::BcDriveEntry entry(Drive_Key);
            entry.setOwner(Owner);
			entry.setSize(Drive_Size);
            entry.setReplicatorCount(Replicator_Count);

            return entry;
        }

		ReplicatorWithAmounts CreateInitialReplicatorWithAmounts(bool acceptable = true) {
            state::ReplicatorEntry entry(test::GenerateRandomByteArray<Key>());
			const auto storageMosaics = acceptable ?
					test::RandomInRange(Drive_Size, Drive_Size+10) :
					test::RandomInRange(0, Drive_Size-1);
			const auto streamingMosaics = acceptable ?
					test::RandomInRange(2*Drive_Size, 2*Drive_Size+10) :
					test::RandomInRange(0, 2*Drive_Size-1);

			return std::make_tuple(entry, Amount(storageMosaics), Amount(streamingMosaics));
        }

		ReplicatorWithAmounts CreateExpectedReplicatorWithAmounts(ReplicatorWithAmounts initialReplicatorWithAmounts) {
			auto& entry = std::get<0>(initialReplicatorWithAmounts);
			auto& storageMosaics = std::get<1>(initialReplicatorWithAmounts);
			auto& streamingMosaics = std::get<2>(initialReplicatorWithAmounts);

			// Here it is suggested that if the replicator has enough mosaics, it will be always assigned to the drive.
			// In fact, if there are more acceptable replicators than it was requested by the drive, then, obviously,
			// some of them won't be assigned.
			if (storageMosaics.unwrap() >= Drive_Size && streamingMosaics.unwrap() >= 2*Drive_Size) {
				entry.drives().emplace(Drive_Key, state::DriveInfo{ Hash256(), false, 0, 0 });
				storageMosaics = Amount(storageMosaics.unwrap() - Drive_Size);
				streamingMosaics = Amount(streamingMosaics.unwrap() - 2*Drive_Size);
			}

			return initialReplicatorWithAmounts;
		}

        struct CacheValues {
            public:
			    explicit CacheValues()
                    : InitialReplicatorsWithAmounts()
					, ExpectedReplicatorsWithAmounts()
					, ExpectedBcDriveEntry(Key())
				{}

            public:
                std::vector<ReplicatorWithAmounts> InitialReplicatorsWithAmounts;
				std::vector<ReplicatorWithAmounts> ExpectedReplicatorsWithAmounts;
				state::BcDriveEntry ExpectedBcDriveEntry;
        };
        
        void RunTest(NotifyMode mode,
					 const CacheValues& values,
					 const size_t expectedDriveQueueSize,
					 const Height& currentHeight) {
            ObserverTestContext context(mode, currentHeight, CreateConfig());
            Notification notification(
				Owner,
				Drive_Key,
				Drive_Size,
				Replicator_Count);
            auto pObserver = CreatePrepareDriveObserver();
            auto& driveCache = context.cache().sub<cache::BcDriveCache>();
			auto& priorityQueueCache = context.cache().sub<cache::PriorityQueueCache>();
            auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();
			auto treeAdapter = CreateAvlTreeAdapter(context.observerContext());

            // Populate cache.
			test::AddAccountState(accountStateCache, values.ExpectedBcDriveEntry.key(), Current_Height);
			for (const auto& tuple : values.InitialReplicatorsWithAmounts) {
				const auto& entry = std::get<0>(tuple);
				replicatorCache.insert(entry);
				test::AddAccountState(accountStateCache, entry.key(), Current_Height,
						{{Storage_Mosaic_Id, std::get<1>(tuple)}, {Streaming_Mosaic_Id, std::get<2>(tuple)}}
 				);
				treeAdapter.insert(entry.key());
			}

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto driveIter = driveCache.find(values.ExpectedBcDriveEntry.key());
            const auto& actualEntry = driveIter.get();
            test::AssertEqualBcDriveData(values.ExpectedBcDriveEntry, actualEntry);

            for (auto i = 0u; i < Total_Replicator_Count; ++i) {
				const auto& expectedEntry = std::get<0>(values.ExpectedReplicatorsWithAmounts.at(i));

				auto replicatorIter = replicatorCache.find(expectedEntry.key());
				auto& replicatorEntry = replicatorIter.get();
				test::AssertEqualReplicatorData(expectedEntry, replicatorEntry);

				auto replicatorStateIter = accountStateCache.find(expectedEntry.key());
				auto& replicatorState = replicatorStateIter.get();
				EXPECT_EQ(replicatorState.Balances.get(Storage_Mosaic_Id), std::get<1>(values.ExpectedReplicatorsWithAmounts.at(i)));
				EXPECT_EQ(replicatorState.Balances.get(Streaming_Mosaic_Id), std::get<2>(values.ExpectedReplicatorsWithAmounts.at(i)));
			}

            auto& paymentQueueCache = context.cache().sub<cache::QueueCache>();
			int paymentQueueSize = 0;

			auto* pQueueEntry = paymentQueueCache.find(state::DrivePaymentQueueKey).tryGet();
			if (pQueueEntry && pQueueEntry->getFirst() != Key()) {
				paymentQueueSize = 1;
			}

			// EXPECT_NE(expectedDriveQueueSize, paymentQueueSize);
			auto& driveQueueEntry = getPriorityQueueEntry(priorityQueueCache, state::DrivePriorityQueueKey);
			EXPECT_EQ(expectedDriveQueueSize, driveQueueEntry.priorityQueue().size());
        }
    }

    TEST(TEST_CLASS, PrepareDrive_Commit_ExactReplicatorCount) {
        // Arrange:
        CacheValues values;
        values.ExpectedBcDriveEntry = CreateBcDriveEntry();
		for (auto i = 0u; i < Total_Replicator_Count; ++i) {
			const bool acceptable = i < Replicator_Count;	// First (Replicator_Count) replicators will have enough mosaics
															// and are expected to be assigned to the drive.
			const auto replicatorWithAmounts = CreateInitialReplicatorWithAmounts(acceptable);
			values.InitialReplicatorsWithAmounts.push_back(replicatorWithAmounts);
			values.ExpectedReplicatorsWithAmounts.push_back(CreateExpectedReplicatorWithAmounts(replicatorWithAmounts));
			if (acceptable)
				values.ExpectedBcDriveEntry.replicators().emplace(std::get<0>(replicatorWithAmounts).key());
		}
		const auto expectedDriveQueueSize = 0;

        // Assert:
        RunTest(NotifyMode::Commit, values, expectedDriveQueueSize, Current_Height);
    }

	TEST(TEST_CLASS, PrepareDrive_Commit_InsufficientReplicatorCount) {
		// Arrange:
		CacheValues values;
		values.ExpectedBcDriveEntry = CreateBcDriveEntry();
		for (auto i = 0u; i < Total_Replicator_Count; ++i) {
			const bool acceptable = i < Replicator_Count - 1;	// First (Replicator_Count - 1) replicators will have enough mosaics
																// and are expected to be assigned to the drive.
			const auto replicatorWithAmounts = CreateInitialReplicatorWithAmounts(acceptable);
			values.InitialReplicatorsWithAmounts.push_back(replicatorWithAmounts);
			values.ExpectedReplicatorsWithAmounts.push_back(CreateExpectedReplicatorWithAmounts(replicatorWithAmounts));
			if (acceptable)
				values.ExpectedBcDriveEntry.replicators().emplace(std::get<0>(replicatorWithAmounts).key());
		}
		const auto expectedDriveQueueSize = 1;

		// Assert:
		RunTest(NotifyMode::Commit, values, expectedDriveQueueSize, Current_Height);
	}

	TEST(TEST_CLASS, PrepareDrive_Commit_NoAcceptableReplicators) {
		// Arrange:
		CacheValues values;
		values.ExpectedBcDriveEntry = CreateBcDriveEntry();
		for (auto i = 0u; i < Total_Replicator_Count; ++i) {
			const auto replicatorWithAmounts = CreateInitialReplicatorWithAmounts(false);
			values.InitialReplicatorsWithAmounts.push_back(replicatorWithAmounts);
			values.ExpectedReplicatorsWithAmounts.push_back(CreateExpectedReplicatorWithAmounts(replicatorWithAmounts));
		}
		const auto expectedDriveQueueSize = 1;

		// Assert:
		RunTest(NotifyMode::Commit, values, expectedDriveQueueSize, Current_Height);
	}

	TEST(TEST_CLASS, PrepareDrive_Commit_NoReplicators) {
		// Arrange:
		CacheValues values;
		values.ExpectedBcDriveEntry = CreateBcDriveEntry();
		for (auto i = 0u; i < Total_Replicator_Count; ++i) {
			const auto replicatorWithAmounts = CreateInitialReplicatorWithAmounts(false);
			values.InitialReplicatorsWithAmounts.push_back(replicatorWithAmounts);
			values.ExpectedReplicatorsWithAmounts.push_back(CreateExpectedReplicatorWithAmounts(replicatorWithAmounts));
		}
		const auto expectedDriveQueueSize = 1;

		// Assert:
		RunTest(NotifyMode::Commit, values, expectedDriveQueueSize, Current_Height);
	}

    TEST(TEST_CLASS, PrepareDrive_Rollback) {
        // Arrange:
        CacheValues values;

        // Assert:
        EXPECT_THROW(RunTest(NotifyMode::Rollback, values, 0, Current_Height), catapult_runtime_error);
    }

}}