/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	namespace {
		void AssertEqualPayments(const std::vector<state::PaymentInformation>& expectedPayments, const std::vector<state::PaymentInformation>& payments) {
			ASSERT_EQ(expectedPayments.size(), payments.size());
			for (auto i = 0u; i < payments.size(); ++i) {
				const auto &expectedPayment = expectedPayments[i];
				const auto &payment = payments[i];
				EXPECT_EQ(expectedPayment.Receiver, payment.Receiver);
				EXPECT_EQ(expectedPayment.Amount, payment.Amount);
				EXPECT_EQ(expectedPayment.Height, payment.Height);
			}
		}
	}
	state::SuperContractEntry CreateSuperContractEntry(
			Key key,
			state::SuperContractState state,
			Key owner,
			Height start,
			Height end,
			VmVersion vmVersion,
			Key mainDriveKey,
			Hash256 fileHash,
			uint16_t executionCount) {
		state::SuperContractEntry entry(key);
		entry.setState(state);
		entry.setOwner(owner);
		entry.setStart(start);
		entry.setEnd(end);
		entry.setVmVersion(vmVersion);
		entry.setMainDriveKey(mainDriveKey);
		entry.setFileHash(fileHash);
		entry.setExecutionCount(executionCount);

		return entry;
	}

	void AssertEqualSuperContractData(const state::SuperContractEntry& entry1, const state::SuperContractEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		EXPECT_EQ(entry1.state(), entry2.state());
		EXPECT_EQ(entry1.owner(), entry2.owner());
		EXPECT_EQ(entry1.start(), entry2.start());
		EXPECT_EQ(entry1.end(), entry2.end());
		EXPECT_EQ(entry1.vmVersion(), entry2.vmVersion());
		EXPECT_EQ(entry1.mainDriveKey(), entry2.mainDriveKey());
		EXPECT_EQ(entry1.fileHash(), entry2.fileHash());
		EXPECT_EQ(entry1.executionCount(), entry2.executionCount());

	}

	template<typename T>
	void AssertEqualReplicators(const T& expectedReplicators, const T& replicators) {
		ASSERT_EQ(expectedReplicators.size(), replicators.size());
		auto expectedReplicatorIter = expectedReplicators.begin();
		auto replicatorIter = replicators.begin();
		for (; replicatorIter != replicators.end(); ++replicatorIter, ++expectedReplicatorIter) {
			EXPECT_EQ(expectedReplicatorIter->first, replicatorIter->first);
			const auto& expectedReplicator = expectedReplicatorIter->second;
			const auto& replicator = replicatorIter->second;
			EXPECT_EQ(expectedReplicator.Start, replicator.Start);
			EXPECT_EQ(expectedReplicator.End, replicator.End);

			const auto& expectedActiveFilesWithoutDeposit = expectedReplicator.ActiveFilesWithoutDeposit;
			const auto& activeFilesWithoutDeposit = replicator.ActiveFilesWithoutDeposit;
			ASSERT_EQ(expectedActiveFilesWithoutDeposit.size(), activeFilesWithoutDeposit.size());
			for (const auto& fileHash : activeFilesWithoutDeposit) {
				EXPECT_EQ(1, expectedActiveFilesWithoutDeposit.count(fileHash));
			}

			const auto& expectedInactiveFilesWithoutDeposit = expectedReplicator.InactiveFilesWithoutDeposit;
			const auto& inactiveFilesWithoutDeposit = replicator.InactiveFilesWithoutDeposit;
			ASSERT_EQ(expectedInactiveFilesWithoutDeposit.size(), inactiveFilesWithoutDeposit.size());
			for (const auto& pair : inactiveFilesWithoutDeposit) {
				const auto& expectedHeights = expectedInactiveFilesWithoutDeposit.at(pair.first);
				const auto& heights = pair.second;
				ASSERT_EQ(expectedHeights.size(), heights.size());
				for (auto i = 0u; i < heights.size(); ++i)
					EXPECT_EQ(expectedHeights[i], heights[i]);
			}
		}
	}

	void AssertEqualDriveData(const state::DriveEntry& expectedEntry, const state::DriveEntry& entry) {
		EXPECT_EQ(expectedEntry.key(), entry.key());
		EXPECT_EQ(expectedEntry.start(), entry.start());
		EXPECT_EQ(expectedEntry.end(), entry.end());
		EXPECT_EQ(expectedEntry.state(), entry.state());
		EXPECT_EQ(expectedEntry.owner(), entry.owner());
		EXPECT_EQ(expectedEntry.rootHash(), entry.rootHash());
		EXPECT_EQ(expectedEntry.duration(), entry.duration());
		EXPECT_EQ(expectedEntry.billingPeriod(), entry.billingPeriod());
		EXPECT_EQ(expectedEntry.billingPrice(), entry.billingPrice());
		EXPECT_EQ(expectedEntry.version(), entry.version());

		const auto& expectedBillingHistory = expectedEntry.billingHistory();
		const auto& billingHistory = entry.billingHistory();
		ASSERT_EQ(expectedBillingHistory.size(), billingHistory.size());
		for (auto i = 0u; i < billingHistory.size(); ++i) {
			const auto& expectedBillingPeriod = expectedBillingHistory[i];
			const auto& billingPeriod = billingHistory[i];
			EXPECT_EQ(expectedBillingPeriod.Start, billingPeriod.Start);
			EXPECT_EQ(expectedBillingPeriod.End, billingPeriod.End);
			AssertEqualPayments(expectedBillingPeriod.Payments, billingPeriod.Payments);
		}

		EXPECT_EQ(expectedEntry.size(), entry.size());
		EXPECT_EQ(expectedEntry.occupiedSpace(), entry.occupiedSpace());
		EXPECT_EQ(expectedEntry.replicas(), entry.replicas());
		EXPECT_EQ(expectedEntry.minReplicators(), entry.minReplicators());
		EXPECT_EQ(expectedEntry.percentApprovers(), entry.percentApprovers());

		const auto& expectedFiles = expectedEntry.files();
		const auto& files = entry.files();
		ASSERT_EQ(expectedFiles.size(), files.size());
		for (const auto& filePair : files) {
			const auto& expectedFile = expectedFiles.at(filePair.first);
			const auto& file = filePair.second;
			EXPECT_EQ(expectedFile.Size, file.Size);
		}

		AssertEqualReplicators(expectedEntry.replicators(), entry.replicators());
		AssertEqualReplicators(expectedEntry.removedReplicators(), entry.removedReplicators());

		AssertEqualPayments(expectedEntry.uploadPayments(), entry.uploadPayments());
	}
}}


