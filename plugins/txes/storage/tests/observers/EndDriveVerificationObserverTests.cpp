/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/StorageNotifications.h"
#include "src/observers/Observers.h"
#include "src/utils/AVLTree.h"
#include "tests/test/StorageTestUtils.h"
#include "tests/test/other/mocks/MockStorageState.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace observers {

#define TEST_CLASS EndDriveVerificationObserverTests

	const std::unique_ptr<observers::LiquidityProviderExchangeObserver>  Liquidity_Provider = std::make_unique<test::LiquidityProviderExchangeObserverImpl>();

    DEFINE_COMMON_OBSERVER_TESTS(EndDriveVerification, Liquidity_Provider, nullptr)

    namespace {
        using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
        using Notification = model::EndDriveVerificationNotification<1>;

        constexpr auto Current_Height = Height(10);
		constexpr auto Drive_Size = 20;
		constexpr auto Shard_Size = 4;

		const auto Verification_Trigger = test::GenerateRandomByteArray<Hash256>();
		const auto Expiration = test::GenerateRandomValue<Timestamp>();
		const auto Duration = test::Random16();
		const auto Seed = test::GenerateRandomByteArray<Hash256>();

		const auto Zero_Key = Key();
		const auto Drive_Key = test::GenerateRandomByteArray<Key>();
		const auto Replicator_Key_1 = Key({1});
		const auto Replicator_Key_2 = Key({2});
		const auto Replicator_Key_3 = Key({3});
		const auto Replicator_Key_4 = Key({4});
		constexpr auto Key_Count = 4;	// Number of replicator keys above

		constexpr auto Currency_Mosaic_Id = MosaicId(1);
		constexpr auto Storage_Mosaic_Id = MosaicId(2);
		constexpr auto Streaming_Mosaic_Id = MosaicId(3);


		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;
			config.Immutable.CurrencyMosaicId = Currency_Mosaic_Id;
			config.Immutable.StorageMosaicId = Storage_Mosaic_Id;
			config.Immutable.StreamingMosaicId = Streaming_Mosaic_Id;

			auto pluginConfig = config::StorageConfiguration::Uninitialized();
			pluginConfig.ShardSize = Shard_Size;
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

		state::Verification CreateVerificationWithEmptyShards(const uint16_t& shardsCount) {
			return state::Verification{ Verification_Trigger, Expiration, Duration, state::Shards(shardsCount) };
		}

		std::optional<state::Verification> CreateExpectedVerification(state::Verification initialVerification, const uint16_t& targetShardId) {
			auto expectedVerification = std::optional<state::Verification>(std::move(initialVerification));
			auto& shards = expectedVerification->Shards;
			shards.at(targetShardId).clear();

			bool completed = true;
			for (const auto& shard : shards) {
				if (!shard.empty()) {
					completed = false;
					break;
				}
			}
			if (completed)
				expectedVerification.reset();

			return expectedVerification;
		}


		using Amounts = std::tuple<uint64_t, uint64_t, uint64_t>;	// XPX, SO units, SM units respectively

		struct CacheValues {
			state::Verification InitialVerification;
			std::set<Key> InitialReplicators;
			std::vector<Key> InitialOffboardingReplicators;
			std::map<Key, Amounts> InitialAmounts;
			std::optional<state::Verification> ExpectedVerification;
			std::set<Key> ExpectedReplicators;
			std::map<Key, Amounts> ExpectedAmounts;
		};

		struct OpinionValues {
			std::vector<Key> JudgingKeys;
			std::vector<std::vector<bool>> Opinions;
		};

        void RunTest(NotifyMode mode,
					 const CacheValues& cacheValues,
					 const OpinionValues& opinionValues,
					 const uint16_t& shardId,
					 const Height& currentHeight) {
            // Arrange:
            ObserverTestContext context(mode, currentHeight, CreateConfig());

            auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
            auto& replicatorCache = context.cache().sub<cache::ReplicatorCache>();
			auto& accountStateCache = context.cache().sub<cache::AccountStateCache>();
			auto treeAdapter = CreateAvlTreeAdapter(context.observerContext());

			// Populate cache.
            state::BcDriveEntry driveEntry(Drive_Key);
			driveEntry.setSize(Drive_Size);
			driveEntry.setReplicatorCount(Key_Count);
            driveEntry.verification() = cacheValues.InitialVerification;
			for (const auto& key : cacheValues.InitialReplicators)
				driveEntry.replicators().insert(key);
			for (const auto& key : cacheValues.InitialOffboardingReplicators)
				driveEntry.offboardingReplicators().emplace_back(key);
			bcDriveCache.insert(driveEntry);

			for (const auto& [key, amounts] : cacheValues.InitialAmounts) {
				std::vector<model::Mosaic> mosaics = {
						{Currency_Mosaic_Id, Amount(std::get<0>(amounts))},
						{Storage_Mosaic_Id, Amount(std::get<1>(amounts))},
						{Streaming_Mosaic_Id, Amount(std::get<2>(amounts))}
				};
				test::AddAccountState(accountStateCache, key, Current_Height, mosaics);
			}

			for (const auto& key : cacheValues.InitialReplicators) {
				state::ReplicatorEntry replicatorEntry(key);
				replicatorEntry.drives()[Drive_Key] = {};
				replicatorCache.insert(replicatorEntry);
				treeAdapter.insert(key);
			}

			// Prepare pointers for notification.
			const auto& judgingKeyCount = opinionValues.JudgingKeys.size();
			const auto pPublicKeys = std::make_unique<Key[]>(judgingKeyCount);
			for (auto i = 0u; i < judgingKeyCount; ++i)
				pPublicKeys[i] = opinionValues.JudgingKeys.at(i);

			boost::dynamic_bitset<uint8_t> opinions(judgingKeyCount * Key_Count);
			for (auto i = 0; i < Key_Count; ++i)
				for (auto j = 0; j < judgingKeyCount; ++j)
					opinions[j * Key_Count + i] = opinionValues.Opinions.at(j).at(i);

			const auto opinionByteCount = (judgingKeyCount * Key_Count + 7u) / 8u;
			std::vector<uint8_t> buffer(opinionByteCount, 0u);
			boost::to_block_range(opinions, buffer.data());

			Notification notification(
					Drive_Key,
					Seed,
					Verification_Trigger,
					shardId,
					Key_Count,
					judgingKeyCount,
					pPublicKeys.get(),
					nullptr,
					buffer.data()
			);
			auto pStorageState = std::make_shared<mocks::MockStorageState>();

            auto pObserver = CreateEndDriveVerificationObserver(Liquidity_Provider, pStorageState);

            // Act:
            test::ObserveNotification(*pObserver, notification, context);

            // Assert: check the cache
            auto driveIter = bcDriveCache.find(Drive_Key);
            const auto& actualDrive = driveIter.tryGet();
            const auto& actualVerification = actualDrive->verification();

			EXPECT_EQ(cacheValues.ExpectedVerification.has_value(), actualVerification.has_value());
			if (actualVerification.has_value()) {
				const auto& expectedVerificationValue = cacheValues.ExpectedVerification.value();
				const auto& actualVerificationValue = actualVerification.value();
				EXPECT_EQ(expectedVerificationValue.VerificationTrigger, actualVerificationValue.VerificationTrigger);
				EXPECT_EQ(expectedVerificationValue.Expiration, actualVerificationValue.Expiration);
				EXPECT_EQ(expectedVerificationValue.Duration, actualVerificationValue.Duration);
				EXPECT_EQ(expectedVerificationValue.Shards, actualVerificationValue.Shards);
			}

			EXPECT_EQ(cacheValues.ExpectedReplicators, actualDrive->replicators());

			for (const auto& [key, amounts] : cacheValues.ExpectedAmounts) {
				auto accountStateIter = accountStateCache.find(key);
				const auto& balances = accountStateIter.get().Balances;
				EXPECT_EQ(Amount(std::get<0>(amounts)), balances.get(Currency_Mosaic_Id));
				EXPECT_EQ(Amount(std::get<1>(amounts)), balances.get(Storage_Mosaic_Id));
				EXPECT_EQ(Amount(std::get<2>(amounts)), balances.get(Streaming_Mosaic_Id));
			}
        }
    }

    TEST(TEST_CLASS, EndDriveVerification_Commit_Verification_Resets) {
        // Arrange:
        CacheValues cacheValues;

		const auto shardsCount = 3;
		const auto shardId = 1;

		cacheValues.InitialVerification = CreateVerificationWithEmptyShards(shardsCount);
		cacheValues.InitialVerification.Shards.at(shardId) = {Replicator_Key_1, Replicator_Key_2, Replicator_Key_3, Replicator_Key_4};
		cacheValues.InitialReplicators = {Replicator_Key_1, Replicator_Key_2, Replicator_Key_3, Replicator_Key_4};
		cacheValues.InitialOffboardingReplicators = {Replicator_Key_3};
		cacheValues.InitialAmounts = {
				{Zero_Key, {0, 80, 0}},
				{Drive_Key, {100, 100, 100}},
				{Replicator_Key_1, {100, 100, 100}},
				{Replicator_Key_2, {100, 100, 100}},
				{Replicator_Key_3, {100, 10, 10}},
				{Replicator_Key_4, {100, 100, 100}},
		};

		OpinionValues opinionValues;
		opinionValues.JudgingKeys = {Replicator_Key_3, Replicator_Key_2};
		opinionValues.Opinions = {
				{true, true, true, false},
				{true, true, true, false},
		};

		cacheValues.ExpectedVerification = CreateExpectedVerification(cacheValues.InitialVerification, shardId);
		cacheValues.ExpectedReplicators = {Replicator_Key_1, Replicator_Key_2};
		cacheValues.ExpectedAmounts = {
				{Zero_Key, {0, 40, 0}},				// (0, -10*2, 0) for deposit slashing after RK4 offboarding,
													 	// (0, -20, 0) for SO refunds to RK3,
				{Drive_Key, {100, 100, 60}},			// (0, 0, -40) for SM refunds to RK3,
				{Replicator_Key_1, {110, 100, 100}},	// (+10, 0, 0) for deposit slashing after RK4 offboarding,
				{Replicator_Key_2, {110, 100, 100}},	// (+10, 0, 0) for deposit slashing after RK4 offboarding,
				{Replicator_Key_3, {160, 10, 10}},	// (+20, 0, 0) for SO refunds from the void account,
													 	// (+40, 0, 0) for SM refunds from the drive,
				{Replicator_Key_4, {100, 100, 100}},	// Failed verification, gets nothing in return
		};

        // Assert
        RunTest(NotifyMode::Commit, cacheValues, opinionValues, shardId, Current_Height);
    }

	TEST(TEST_CLASS, EndDriveVerification_Commit_Verification_Remains) {
		// Arrange:
		CacheValues cacheValues;

		const auto shardsCount = 3;
		const auto shardId = 1;

		cacheValues.InitialVerification = CreateVerificationWithEmptyShards(shardsCount);
		cacheValues.InitialVerification.Shards.at(shardId) = {Replicator_Key_1, Replicator_Key_2, Replicator_Key_3, Replicator_Key_4};

		// Add another non-empty shard
		cacheValues.InitialVerification.Shards.at(0).insert(test::GenerateRandomByteArray<Key>());

		cacheValues.InitialReplicators = {Replicator_Key_1, Replicator_Key_2, Replicator_Key_3, Replicator_Key_4};
		cacheValues.InitialOffboardingReplicators = {Replicator_Key_3};
		cacheValues.InitialAmounts = {
				{Zero_Key, {0, 80, 0}},
				{Drive_Key, {100, 100, 100}},
				{Replicator_Key_1, {100, 100, 100}},
				{Replicator_Key_2, {100, 100, 100}},
				{Replicator_Key_3, {100, 10, 10}},
				{Replicator_Key_4, {100, 100, 100}},
		};

		OpinionValues opinionValues;
		opinionValues.JudgingKeys = {Replicator_Key_3, Replicator_Key_2};
		opinionValues.Opinions = {
				{true, true, true, false},
				{true, true, true, false},
		};

		cacheValues.ExpectedVerification = CreateExpectedVerification(cacheValues.InitialVerification, shardId);
		cacheValues.ExpectedReplicators = {Replicator_Key_1, Replicator_Key_2};
		cacheValues.ExpectedAmounts = {
				{Zero_Key, {0, 40, 0}},				// (0, -10*2, 0) for deposit slashing after RK4 offboarding,
													 	// (0, -20, 0) for SO refunds to RK3,
				{Drive_Key, {100, 100, 60}},			// (0, 0, -40) for SM refunds to RK3,
				{Replicator_Key_1, {110, 100, 100}},	// (+10, 0, 0) for deposit slashing after RK4 offboarding,
				{Replicator_Key_2, {110, 100, 100}},	// (+10, 0, 0) for deposit slashing after RK4 offboarding,
				{Replicator_Key_3, {160, 10, 10}},	// (+20, 0, 0) for SO refunds from the void account,
													 	// (+40, 0, 0) for SM refunds from the drive,
				{Replicator_Key_4, {100, 100, 100}},	// Failed verification, gets nothing in return
		};

		// Assert
		RunTest(NotifyMode::Commit, cacheValues, opinionValues, shardId, Current_Height);
	}

    TEST(TEST_CLASS, EndDriveVerification_Rollback) {
        // Arrange:
		CacheValues cacheValues;
		OpinionValues opinionValues;

		const auto shardId = 1;

        // Assert
        EXPECT_THROW(RunTest(NotifyMode::Rollback, cacheValues, opinionValues, shardId, Current_Height), catapult_runtime_error);
    }

}}