/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/DriveEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS DriveEntryTests

	TEST(TEST_CLASS, CanCreateDriveEntry) {
		// Act:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = DriveEntry(key);

		// Assert:
		EXPECT_EQ(key, entry.key());
	}

	TEST(TEST_CLASS, CanAccessStart) {
		// Arrange:
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(Height(0), entry.start());

		// Act:
		entry.setStart(Height(10));

		// Assert:
		EXPECT_EQ(Height(10), entry.start());
	}

	TEST(TEST_CLASS, CanAccessEnd) {
		// Arrange:
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(Height(0), entry.end());

		// Act:
		entry.setEnd(Height(10));

		// Assert:
		EXPECT_EQ(Height(10), entry.end());
	}

	TEST(TEST_CLASS, CanAccessState) {
		// Arrange:
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(DriveState::NotStarted, entry.state());

		// Act:
		entry.setState(DriveState::InProgress);

		// Assert:
		EXPECT_EQ(DriveState::InProgress, entry.state());
	}

	TEST(TEST_CLASS, CanAccessOwner) {
		// Arrange:
		auto owner = test::GenerateRandomByteArray<Key>();
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(Key(), entry.owner());

		// Act:
		entry.setOwner(owner);

		// Assert:
		EXPECT_EQ(owner, entry.owner());
	}

	TEST(TEST_CLASS, CanAccessRootHash) {
		// Arrange:
		auto rootHash = test::GenerateRandomByteArray<Hash256>();
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(Hash256(), entry.rootHash());

		// Act:
		entry.setRootHash(rootHash);

		// Assert:
		EXPECT_EQ(rootHash, entry.rootHash());
	}

	TEST(TEST_CLASS, CanAccessDuration) {
		// Arrange:
		auto duration = BlockDuration(10);
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(BlockDuration(0), entry.duration());

		// Act:
		entry.setDuration(duration);

		// Assert:
		EXPECT_EQ(duration, entry.duration());
	}

	TEST(TEST_CLASS, CanAccessBillingPeriod) {
		// Arrange:
		auto billingPeriod = BlockDuration(10);
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(BlockDuration(0), entry.billingPeriod());

		// Act:
		entry.setBillingPeriod(billingPeriod);

		// Assert:
		EXPECT_EQ(billingPeriod, entry.billingPeriod());
	}

	TEST(TEST_CLASS, CanAccessBillingPrice) {
		// Arrange:
		auto billingPrice = Amount(10);
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(Amount(0), entry.billingPrice());

		// Act:
		entry.setBillingPrice(billingPrice);

		// Assert:
		EXPECT_EQ(billingPrice, entry.billingPrice());
	}

	TEST(TEST_CLASS, CanAccessSize) {
		// Arrange:
		uint64_t size = 100u;
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(0u, entry.size());

		// Act:
		entry.setSize(size);

		// Assert:
		EXPECT_EQ(size, entry.size());
	}

	TEST(TEST_CLASS, CanAccessOccupiedSpace) {
		// Arrange:
		uint64_t occupiedSpace = 100u;
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(0u, entry.occupiedSpace());

		// Act:
		entry.setOccupiedSpace(occupiedSpace);

		// Assert:
		EXPECT_EQ(occupiedSpace, entry.occupiedSpace());
	}

	TEST(TEST_CLASS, CannotIncreaseOccupiedSpaceWhenOverflows) {
		// Arrange:
		uint64_t occupiedSpace = 100u;
		uint64_t delta = std::numeric_limits<uint64_t>::max();
		auto entry = DriveEntry(Key());
		entry.setOccupiedSpace(occupiedSpace);

		// Sanity:
		EXPECT_EQ(occupiedSpace, entry.occupiedSpace());

		// Act + Assert:
		EXPECT_THROW(entry.increaseOccupiedSpace(delta), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CanIncreaseOccupiedSpace) {
		// Arrange:
		uint64_t occupiedSpace = 100u;
		uint64_t delta = 10u;
		auto entry = DriveEntry(Key());
		entry.setOccupiedSpace(occupiedSpace);

		// Sanity:
		EXPECT_EQ(occupiedSpace, entry.occupiedSpace());

		// Act:
		entry.increaseOccupiedSpace(delta);

		// Assert:
		EXPECT_EQ(occupiedSpace + delta, entry.occupiedSpace());
	}

	TEST(TEST_CLASS, CannotDecreaseOccupiedSpaceWhenDeltaTooBig) {
		// Arrange:
		uint64_t occupiedSpace = 100u;
		uint64_t delta = 101u;
		auto entry = DriveEntry(Key());
		entry.setOccupiedSpace(occupiedSpace);

		// Sanity:
		EXPECT_EQ(occupiedSpace, entry.occupiedSpace());

		// Act + Assert:
		EXPECT_THROW(entry.decreaseOccupiedSpace(delta), catapult_invalid_argument);
	}

	TEST(TEST_CLASS, CanDecreaseOccupiedSpace) {
		// Arrange:
		uint64_t occupiedSpace = 100u;
		uint64_t delta = 10u;
		auto entry = DriveEntry(Key());
		entry.setOccupiedSpace(occupiedSpace);

		// Sanity:
		EXPECT_EQ(occupiedSpace, entry.occupiedSpace());

		// Act:
		entry.decreaseOccupiedSpace(delta);

		// Assert:
		EXPECT_EQ(occupiedSpace - delta, entry.occupiedSpace());
	}

	TEST(TEST_CLASS, CanAccessReplicas) {
		// Arrange:
		uint16_t replicas = 10u;
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(0u, entry.replicas());

		// Act:
		entry.setReplicas(replicas);

		// Assert:
		EXPECT_EQ(replicas, entry.replicas());
	}

	TEST(TEST_CLASS, CanAccessMinReplicators) {
		// Arrange:
		uint16_t minReplicators = 10u;
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(0u, entry.minReplicators());

		// Act:
		entry.setMinReplicators(minReplicators);

		// Assert:
		EXPECT_EQ(minReplicators, entry.minReplicators());
	}

	TEST(TEST_CLASS, CanAccessPercentApprovers) {
		// Arrange:
		uint16_t percentApprovers = 10u;
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_EQ(0u, entry.percentApprovers());

		// Act:
		entry.setPercentApprovers(percentApprovers);

		// Assert:
		EXPECT_EQ(percentApprovers, entry.percentApprovers());
	}

	TEST(TEST_CLASS, CanAccessBillingHistory) {
		// Arrange:
		std::vector<Key> receivers{test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>()};
		BillingPeriodDescription billingPeriod{Height(5), Height(20),
			{
				{ receivers[0], Amount(100), Height(8) },
				{ receivers[1], Amount(200), Height(16) }
			}
		};
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_TRUE(entry.billingHistory().empty());

		// Act:
		entry.billingHistory().emplace_back(billingPeriod);

		// Assert:
		ASSERT_EQ(1, entry.billingHistory().size());
		EXPECT_EQ(Height(5), entry.billingHistory().back().Start);
		EXPECT_EQ(Height(20), entry.billingHistory().back().End);
		ASSERT_EQ(2, entry.billingHistory().back().Payments.size());
		for (auto i = 0u; i < receivers.size(); ++i) {
			const auto& payment = entry.billingHistory().back().Payments[i];
			EXPECT_EQ(receivers[i], payment.Receiver);
			EXPECT_EQ(Amount(100 * (i + 1)), payment.Amount);
			EXPECT_EQ(Height(8 * (i + 1)), payment.Height);
		}
	}

	TEST(TEST_CLASS, ProcessedDurationReturnsZeroWhenBillingHistoryEmpty) {
		// Arrange:
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_TRUE(entry.billingHistory().empty());

		// Act:
		auto processedDuration = entry.processedDuration();

		// Assert:
		EXPECT_EQ(BlockDuration(0), processedDuration);
	}

	TEST(TEST_CLASS, ProcessedDurationReturnsCorrectResult) {
		// Arrange:
		auto entry = DriveEntry(Key());
		uint64_t heightIncrement = 5;
		uint64_t historySize = 10;
		for (auto i = 1u; i <= historySize; ++i) {
			entry.billingHistory().emplace_back(
				BillingPeriodDescription{
					Height(heightIncrement * i + 1),
					Height(heightIncrement * (i + 1)),
					{ { Key(), Amount(100), Height(1) } }
				}
			);
		}

		// Act:
		auto processedDuration = entry.processedDuration();

		// Assert:
		EXPECT_EQ(BlockDuration(historySize * (heightIncrement - 1)), processedDuration);
	}

	TEST(TEST_CLASS, CanAccessFiles) {
		// Arrange:
		std::vector<Hash256> fileHashes{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		std::vector<Key> receivers{ test::GenerateRandomByteArray<Key>(), test::GenerateRandomByteArray<Key>() };
		std::vector<bool> isActive{ false, true };
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_TRUE(entry.files().empty());

		// Act:
		for (auto i = 1u; i <= fileHashes.size(); ++i) {
			entry.files().emplace(fileHashes[i - 1], FileInfo{i * 1234u});
		}

		// Assert:
		ASSERT_EQ(2, entry.files().size());
		for (auto i = 1u; i <= fileHashes.size(); ++i) {
			auto& fileInfo = entry.files().at(fileHashes[i - 1]);
			EXPECT_EQ(i * 1234u, fileInfo.Size);
		}
	}

	TEST(TEST_CLASS, CanAccessReplicators) {
		// Arrange:
		std::set<Hash256> fileHashes{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		std::vector<Hash256> removedFileHashes{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_TRUE(entry.replicators().empty());

		// Act:
		entry.replicators().emplace(replicatorKey, ReplicatorInfo{ Height(2), Height(4), fileHashes,
			{
				{ removedFileHashes[0], { Height(1) } },
				{ removedFileHashes[1], { Height(1), Height(2) } },
				{ removedFileHashes[2], { Height(1), Height(2), Height(3) } },
			}
		});

		// Assert:
		ASSERT_EQ(1, entry.replicators().size());
		const auto& replicatorInfo = entry.replicators().at(replicatorKey);
		EXPECT_EQ(Height(2), replicatorInfo.Start);
		EXPECT_EQ(Height(4), replicatorInfo.End);
		ASSERT_EQ(fileHashes, replicatorInfo.ActiveFilesWithoutDeposit);
		for (auto i = 0u; i < removedFileHashes.size(); ++i) {
			const auto& heights = replicatorInfo.InactiveFilesWithoutDeposit.at(removedFileHashes[i]);
			for (auto k = 0u; k <= i; ++k)
				EXPECT_EQ(Height(k + 1), heights[k]);
		}
	}

	TEST(TEST_CLASS, ReplicatorInfo_AddInactiveUndepositedFile) {
		// Arrange:
		auto fileHash =  test::GenerateRandomByteArray<Hash256>();
		ReplicatorInfo replicatorInfo;
		// Sanity:
		ASSERT_EQ(0, replicatorInfo.InactiveFilesWithoutDeposit.size());

		// Act:
		replicatorInfo.AddInactiveUndepositedFile(fileHash, Height(10));

		// Assert:
		ASSERT_EQ(1, replicatorInfo.InactiveFilesWithoutDeposit.size());
		ASSERT_EQ(1, replicatorInfo.InactiveFilesWithoutDeposit.at(fileHash).size());
		EXPECT_EQ(Height(10), replicatorInfo.InactiveFilesWithoutDeposit.at(fileHash)[0]);
	}

	TEST(TEST_CLASS, ReplicatorInfo_RemoveInactiveUndepositedFile) {
		// Arrange:
		auto fileHash1 =  test::GenerateRandomByteArray<Hash256>();
		auto fileHash2 =  test::GenerateRandomByteArray<Hash256>();
		ReplicatorInfo replicatorInfo;
		replicatorInfo.AddInactiveUndepositedFile(fileHash1, Height(10));
		replicatorInfo.AddInactiveUndepositedFile(fileHash2, Height(20));
		// Sanity:
		ASSERT_EQ(2, replicatorInfo.InactiveFilesWithoutDeposit.size());

		// Act:
		replicatorInfo.RemoveInactiveUndepositedFile(fileHash1, Height(10));
		replicatorInfo.RemoveInactiveUndepositedFile(fileHash2, Height(10));

		// Assert:
		ASSERT_EQ(1, replicatorInfo.InactiveFilesWithoutDeposit.size());
		ASSERT_EQ(1, replicatorInfo.InactiveFilesWithoutDeposit.at(fileHash2).size());
		EXPECT_EQ(Height(20), replicatorInfo.InactiveFilesWithoutDeposit.at(fileHash2)[0]);
	}

	TEST(TEST_CLASS, CanAccessRemovedReplicators) {
		// Arrange:
		std::set<Hash256> fileHashes{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		std::vector<Hash256> removedFileHashes{ test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>(), test::GenerateRandomByteArray<Hash256>() };
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		auto entry = DriveEntry(Key());

		// Sanity:
		ASSERT_TRUE(entry.removedReplicators().empty());

		// Act:
		entry.removedReplicators().push_back({replicatorKey, ReplicatorInfo{ Height(2), Height(4), fileHashes,
			{
				{ removedFileHashes[0], { Height(1) } },
				{ removedFileHashes[1], { Height(1), Height(2) } },
				{ removedFileHashes[2], { Height(1), Height(2), Height(3) } },
			}
		}});

		// Assert:
		ASSERT_EQ(1, entry.removedReplicators().size());
		EXPECT_EQ(replicatorKey, entry.removedReplicators()[0].first);
		const auto& replicatorInfo = entry.removedReplicators()[0].second;
		EXPECT_EQ(Height(2), replicatorInfo.Start);
		EXPECT_EQ(Height(4), replicatorInfo.End);
		ASSERT_EQ(fileHashes, replicatorInfo.ActiveFilesWithoutDeposit);
		for (auto i = 0u; i < removedFileHashes.size(); ++i) {
			const auto& heights = replicatorInfo.InactiveFilesWithoutDeposit.at(removedFileHashes[i]);
			for (auto k = 0u; k <= i; ++k)
				EXPECT_EQ(Height(k + 1), heights[k]);
		}
	}

	TEST(TEST_CLASS, CannotRemoveNonexistentReplicator) {
		// Arrange:
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		auto entry = DriveEntry(Key());
		entry.replicators().emplace(replicatorKey, ReplicatorInfo{ Height(2), Height(4), {}, {} });

		// Sanity:
		ASSERT_EQ(1, entry.replicators().size());
		ASSERT_TRUE(entry.removedReplicators().empty());

		// Act:
		EXPECT_THROW(entry.removeReplicator(test::GenerateRandomByteArray<Key>()), catapult_runtime_error);

		// Assert:
		EXPECT_TRUE(entry.removedReplicators().empty());
		ASSERT_EQ(1, entry.replicators().size());
		EXPECT_EQ(replicatorKey, entry.replicators().begin()->first);
	}

	TEST(TEST_CLASS, CanRemoveExistentReplicator) {
		// Arrange:
		auto replicatorKey = test::GenerateRandomByteArray<Key>();
		auto entry = DriveEntry(Key());
		entry.replicators().emplace(replicatorKey, ReplicatorInfo{ Height(2), Height(4), {}, {} });

		// Sanity:
		ASSERT_EQ(1, entry.replicators().size());
		ASSERT_TRUE(entry.removedReplicators().empty());

		// Act:
		entry.removeReplicator(replicatorKey);

		// Assert:
		EXPECT_TRUE(entry.replicators().empty());
		ASSERT_EQ(1, entry.removedReplicators().size());
		EXPECT_EQ(replicatorKey, entry.removedReplicators()[0].first);
	}

	TEST(TEST_CLASS, CannotRestoreInvalidReplicator) {
		// Arrange:
		auto replicatorKey1 = test::GenerateRandomByteArray<Key>();
		auto replicatorKey2 = test::GenerateRandomByteArray<Key>();
		auto entry = DriveEntry(Key());
		entry.removedReplicators().push_back({ replicatorKey1, ReplicatorInfo{ Height(2), Height(4), {}, {} } });
		entry.removedReplicators().push_back({ replicatorKey2, ReplicatorInfo{ Height(2), Height(4), {}, {} } });

		// Sanity:
		ASSERT_TRUE(entry.replicators().empty());
		ASSERT_EQ(2, entry.removedReplicators().size());

		// Act:
		EXPECT_THROW(entry.restoreReplicator(replicatorKey1), catapult_runtime_error);
		EXPECT_THROW(entry.restoreReplicator(test::GenerateRandomByteArray<Key>()), catapult_runtime_error);

		// Assert:
		EXPECT_TRUE(entry.replicators().empty());
		ASSERT_EQ(2, entry.removedReplicators().size());
		EXPECT_EQ(replicatorKey1, entry.removedReplicators()[0].first);
		EXPECT_EQ(replicatorKey2, entry.removedReplicators()[1].first);
	}

	TEST(TEST_CLASS, CanRestoreValidReplicator) {
		// Arrange:
		auto replicatorKey1 = test::GenerateRandomByteArray<Key>();
		auto replicatorKey2 = test::GenerateRandomByteArray<Key>();
		auto entry = DriveEntry(Key());
		entry.removedReplicators().push_back({ replicatorKey1, ReplicatorInfo{ Height(2), Height(4), {}, {} } });
		entry.removedReplicators().push_back({ replicatorKey2, ReplicatorInfo{ Height(2), Height(4), {}, {} } });

		// Sanity:
		ASSERT_TRUE(entry.replicators().empty());
		ASSERT_EQ(2, entry.removedReplicators().size());

		// Act:
		entry.restoreReplicator(replicatorKey2);

		// Assert:
		ASSERT_EQ(1, entry.replicators().size());
		EXPECT_EQ(replicatorKey2, entry.replicators().begin()->first);
		ASSERT_EQ(1, entry.removedReplicators().size());
		EXPECT_EQ(replicatorKey1, entry.removedReplicators()[0].first);
	}
}}
