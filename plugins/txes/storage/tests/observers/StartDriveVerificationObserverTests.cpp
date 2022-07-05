/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tests/test/StorageTestUtils.h"
#include "src/observers/Observers.h"
#include "tests/test/plugins/ObserverTestUtils.h"
#include "tests/test/other/mocks/MockStorageState.h"
#include "tests/TestHarness.h"
#include "src/utils/AVLTree.h"

namespace catapult { namespace observers {

#define TEST_CLASS StartDriveVerificationObserverTests

	namespace {
		using ObserverTestContext = test::ObserverTestContextT<test::BcDriveCacheFactory>;
		using Notification = model::BlockNotification<1>;

		constexpr Height Current_Height(20);

		auto CreateConfig() {
			test::MutableBlockchainConfiguration config;

			auto storageConfig = config::StorageConfiguration::Uninitialized();
			storageConfig.VerificationExpirationCoefficient = 0.06;
			storageConfig.VerificationExpirationConstant = 10;
			storageConfig.VerificationInterval = utils::TimeSpan::FromHours(4);
			storageConfig.ShardSize = 20;
			storageConfig.Enabled = true;

			config.Network.SetPluginConfiguration(storageConfig);

			return config.ToConst();
		}

		struct CacheValues {
		public:
			CacheValues(
					const std::vector<state::BcDriveEntry>& initialBcDriveEntries,
					const std::vector<state::BcDriveEntry>& expectedBcDriveEntries)
				: InitialBcDriveEntries(initialBcDriveEntries), ExpectedBcDriveEntries(expectedBcDriveEntries) {}

		public:
			std::vector<state::BcDriveEntry> InitialBcDriveEntries;
			std::vector<state::BcDriveEntry> ExpectedBcDriveEntries;
		};

		void RunTest(NotifyMode mode, const CacheValues& values, const Height& currentHeight, int rounds) {
			// Arrange:
			auto config = CreateConfig();
			ObserverTestContext context(mode, Current_Height, config);

			auto& bcDriveCache = context.cache().sub<cache::BcDriveCache>();
			auto& queueCache = context.cache().sub<cache::QueueCache>();
			utils::AVLTreeAdapter<Key> treeAdapter(
					context.cache().sub<cache::QueueCache>(),
					state::DriveVerificationsTree,
					[](const Key& key) { return key; },
					[&bcDriveCache](const Key& key) -> state::AVLTreeNode {
						return bcDriveCache.find(key).get().verificationNode();
					},
					[&bcDriveCache](const Key& key, const state::AVLTreeNode& node) {
						bcDriveCache.find(key).get().verificationNode() = node;
					});

			for (const auto& driveEntry : values.InitialBcDriveEntries) {
				bcDriveCache.insert(driveEntry);
				treeAdapter.insert(driveEntry.key());
			}

			// Act:
			for (uint i = 0; i < rounds; i++) {
				Timestamp lastBlockTime(i);
				Timestamp notificationTimestamp(lastBlockTime + Timestamp(15 * 1000));
				Notification notification(
						test::GenerateRandomByteArray<Key>(),
						test::GenerateRandomByteArray<Key>(),
						notificationTimestamp,
						test::GenerateRandomValue<Difficulty>(),
						0,
						0);
				mocks::MockStorageState storageState;
				storageState.setLastBlockElementSupplier([lastBlockTime] {
					model::Block block;
					block.Timestamp = lastBlockTime;
					return std::make_shared<model::BlockElement>(block);
				});
				auto pObserver = CreateStartDriveVerificationObserver(storageState);
				test::ObserveNotification(*pObserver, notification, context);
				lastBlockTime = notificationTimestamp;
			}

			const auto& pluginConfig = config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

			// Assert: check the cache
			for (const auto& expectedEntry : values.ExpectedBcDriveEntries) {
				const auto& actualEntry = bcDriveCache.find(expectedEntry.key()).get();
				ASSERT_EQ(expectedEntry.verification().has_value(), actualEntry.verification().has_value());
				if (expectedEntry.verification()) {
					auto expectedShardsNumber =
							std::max(expectedEntry.replicators().size() / (pluginConfig.ShardSize / 2), 1ul);
					ASSERT_EQ(expectedShardsNumber, actualEntry.verification()->Shards.size());
					if (expectedShardsNumber == 1) {
						ASSERT_EQ(
								expectedEntry.replicators().size(), actualEntry.verification()->Shards.front().size());
					} else {
						auto expectedSmallShardsSize = expectedEntry.replicators().size() / expectedShardsNumber;
						auto expectedLargeShardsSize = expectedSmallShardsSize + 1;
						auto expectedLargeShardsNumber = expectedEntry.replicators().size() % expectedShardsNumber;
						for (auto i = 0; i < expectedLargeShardsNumber; i++) {
							ASSERT_EQ(expectedLargeShardsSize, actualEntry.verification()->Shards[i].size());
						}
						for (auto i = expectedLargeShardsNumber; i < expectedShardsNumber; i++) {
							ASSERT_EQ(expectedSmallShardsSize, actualEntry.verification()->Shards[i].size());
						}
					}
				}
			}
		}
	} // namespace

	TEST(TEST_CLASS, PeriodicStoragePayment_SingleDrive) {
		// Arrange:
		Key key = { { 127 } };

		state::BcDriveEntry initialEntry(key);
		initialEntry.setUsedSizeBytes(utils::FileSize::FromGigabytes(11).bytes());
		initialEntry.setRootHash(test::GenerateRandomByteArray<Hash256>());
		for (int i = 0; i < 123; i++) {
			initialEntry.replicators().insert(test::GenerateRandomByteArray<Key>());
		}
		for (const auto& replicator : initialEntry.replicators()) {
			initialEntry.confirmedStorageInfos()[replicator] = { Timestamp(0), Timestamp(0) };
		}

		auto expectedBcDriveEntry = initialEntry;
		expectedBcDriveEntry.verification() = state::Verification();

		CacheValues values({ initialEntry }, { expectedBcDriveEntry });
		// Assert
		RunTest(NotifyMode::Commit, values, Current_Height, 10000);
	}
}} // namespace catapult::observers