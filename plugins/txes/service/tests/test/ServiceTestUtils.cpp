/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ServiceTestUtils.h"
#include "tests/TestHarness.h"

namespace catapult { namespace test {

	namespace {
		void GeneratePayments(uint16_t count, std::vector<state::PaymentInformation>& payments) {
			payments.reserve(count);
			for (auto i = 0u; i < count; ++i) {
				payments.emplace_back(state::PaymentInformation{test::GenerateRandomByteArray<Key>(), test::GenerateRandomValue<Amount>(), test::GenerateRandomValue<Height>()});
			}
		}

		void GenerateActions(uint16_t count, std::vector<state::DriveAction>& actions) {
			actions.reserve(count);
			for (auto i = 0u; i < count; ++i) {
				actions.emplace_back(state::DriveAction{static_cast<state::DriveActionType>(test::RandomByte()), test::GenerateRandomValue<Height>()});
			}
		}

		state::FileInfo GenerateFileInfo(uint16_t actionCount, uint16_t paymentCount) {
			state::FileInfo fileInfo;
			fileInfo.Size = test::Random();
			fileInfo.Deposit = test::GenerateRandomValue<Amount>();
			GenerateActions(actionCount, fileInfo.Actions);
			GeneratePayments(paymentCount, fileInfo.Payments);

			return fileInfo;
		}

		state::ReplicatorInfo GenerateReplicatorInfo(uint16_t filesWithoutDepositCount) {
			state::ReplicatorInfo replicatorInfo;
			replicatorInfo.Start = test::GenerateRandomValue<Height>();
			replicatorInfo.End = test::GenerateRandomValue<Height>();
			replicatorInfo.Deposit = test::GenerateRandomValue<Amount>();
			for (auto i = 0u; i < filesWithoutDepositCount; ++i) {
				replicatorInfo.FilesWithoutDeposit.emplace(test::GenerateRandomByteArray<Hash256>(), test::Random16());
			}

			return replicatorInfo;
		}

		void AssertEqualPayments(const std::vector<state::PaymentInformation>& expectedPayments, const std::vector<state::PaymentInformation>& payments) {
			EXPECT_EQ(expectedPayments.size(), payments.size());
			for (auto i = 0u; i < payments.size(); ++i) {
				const auto &expectedPayment = expectedPayments[i];
				const auto &payment = payments[i];
				EXPECT_EQ(expectedPayment.Receiver, payment.Receiver);
				EXPECT_EQ(expectedPayment.Amount, payment.Amount);
				EXPECT_EQ(expectedPayment.Height, payment.Height);
			}
		}
	}

	state::DriveEntry CreateDriveEntry(
			Key key,
			uint16_t billingHistoryCount,
			uint16_t drivePaymentCount,
			uint16_t fileCount,
			uint16_t fileActionCount,
			uint16_t filePaymentCount,
			uint16_t replicatorCount,
			uint16_t fileWithoutDepositCount) {
		state::DriveEntry entry(key);
		entry.setStart(test::GenerateRandomValue<Height>());
		entry.setEnd(test::GenerateRandomValue<Height>());
		entry.setState(static_cast<state::DriveState>(test::RandomByte()));
		entry.setOwner(test::GenerateRandomByteArray<Key>());
		entry.setRootHash(test::GenerateRandomByteArray<Hash256>());
		entry.setBillingPeriod(test::GenerateRandomValue<BlockDuration>());
		entry.setBillingPrice(test::GenerateRandomValue<Amount>());

		entry.billingHistory().reserve(billingHistoryCount);
		for (auto i = 0u; i < billingHistoryCount; ++i) {
			entry.billingHistory().emplace_back(state::BillingPeriodDescription{test::GenerateRandomValue<Height>(), test::GenerateRandomValue<Height>(), {}});
			GeneratePayments(drivePaymentCount, entry.billingHistory()[i].Payments);
		}

		entry.setSize(test::Random());
		entry.setReplicas(test::Random16());
		entry.setMinReplicators(test::Random16());
		entry.setPercentApprovers(test::RandomByte());

		for (auto i = 0u; i < fileCount; ++i) {
			entry.files().emplace(test::GenerateRandomByteArray<Hash256>(), GenerateFileInfo(fileActionCount, filePaymentCount));
		}

		for (auto i = 0u; i < replicatorCount; ++i) {
			entry.replicators().emplace(test::GenerateRandomByteArray<Key>(), GenerateReplicatorInfo(fileWithoutDepositCount));
		}

		return entry;
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

		const auto& expectedBillingHistory = expectedEntry.billingHistory();
		const auto& billingHistory = entry.billingHistory();
		EXPECT_EQ(expectedBillingHistory.size(), billingHistory.size());
		for (auto i = 0u; i < billingHistory.size(); ++i) {
			const auto& expectedBillingPeriod = expectedBillingHistory[i];
			const auto& billingPeriod = billingHistory[i];
			EXPECT_EQ(expectedBillingPeriod.Start, billingPeriod.Start);
			EXPECT_EQ(expectedBillingPeriod.End, billingPeriod.End);
			AssertEqualPayments(expectedBillingPeriod.Payments, billingPeriod.Payments);
		}

		EXPECT_EQ(expectedEntry.size(), entry.size());
		EXPECT_EQ(expectedEntry.replicas(), entry.replicas());
		EXPECT_EQ(expectedEntry.minReplicators(), entry.minReplicators());
		EXPECT_EQ(expectedEntry.percentApprovers(), entry.percentApprovers());

		const auto& expectedFiles = expectedEntry.files();
		const auto& files = entry.files();
		EXPECT_EQ(expectedFiles.size(), files.size());
		for (const auto& filePair : files) {
			const auto& expectedFile = expectedFiles.at(filePair.first);
			const auto& file = filePair.second;
			EXPECT_EQ(expectedFile.Size, file.Size);
			EXPECT_EQ(expectedFile.Deposit, file.Deposit);

			const auto& expectedActions = expectedFile.Actions;
			const auto& actions = file.Actions;
			EXPECT_EQ(expectedActions.size(), actions.size());
			for (auto i = 0u; i < actions.size(); ++i) {
				const auto& expectedAction = expectedActions[i];
				const auto& action = actions[i];
				EXPECT_EQ(expectedAction.Type, action.Type);
				EXPECT_EQ(expectedAction.ActionHeight, action.ActionHeight);
			}

			AssertEqualPayments(expectedFile.Payments, file.Payments);
		}

		const auto& expectedReplicators = expectedEntry.replicators();
		const auto& replicators = entry.replicators();
		EXPECT_EQ(expectedReplicators.size(), replicators.size());
		for (const auto& replicatorPair : replicators) {
			const auto& expectedReplicator = expectedReplicators.at(replicatorPair.first);
			const auto& replicator = replicatorPair.second;
			EXPECT_EQ(expectedReplicator.Start, replicator.Start);
			EXPECT_EQ(expectedReplicator.End, replicator.End);
			EXPECT_EQ(expectedReplicator.Deposit, replicator.Deposit);

			const auto& expectedFilesWithoutDeposit = expectedReplicator.FilesWithoutDeposit;
			const auto& filesWithoutDeposit = replicator.FilesWithoutDeposit;
			EXPECT_EQ(expectedFilesWithoutDeposit.size(), filesWithoutDeposit.size());
			for (const auto& pair : filesWithoutDeposit) {
				EXPECT_EQ(expectedFilesWithoutDeposit.at(pair.first), pair.second);
			}
		}
	}
}}


