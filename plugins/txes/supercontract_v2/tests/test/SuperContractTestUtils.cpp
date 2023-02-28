/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractTestUtils.h"

namespace catapult { namespace test {

	state::SuperContractEntry CreateSuperContractEntry(
			Key superContractKey,
			Key driveKey,
			Key superContractOwnerKey,
			Key executionPaymentKey,
			Hash256 deploymentBaseModificationId) {
		state::SuperContractEntry entry(superContractKey);
		entry.setDriveKey(driveKey);
		entry.setAssignee(superContractOwnerKey);
		entry.setExecutionPaymentKey(executionPaymentKey);
		entry.setDeploymentBaseModificationId(deploymentBaseModificationId);

		return entry;
	}

	state::DriveContractEntry CreateDriveContractEntry(Key drive, Key contract) {
		state::DriveContractEntry entry(drive);
		entry.setContractKey(contract);

		return entry;
	}

	void AssertEqualAutomaticExecutionsInfo(const state::AutomaticExecutionsInfo& entry1, const state::AutomaticExecutionsInfo& entry2) {
		EXPECT_EQ(entry1.AutomatedExecutionsNumber, entry2.AutomatedExecutionsNumber);
		EXPECT_EQ(entry1.AutomaticDownloadCallPayment, entry2.AutomaticDownloadCallPayment);
		EXPECT_EQ(entry1.AutomaticExecutionCallPayment, entry2.AutomaticExecutionCallPayment);
		EXPECT_EQ(entry1.AutomaticExecutionFileName, entry2.AutomaticExecutionFileName);
		EXPECT_EQ(entry1.AutomaticExecutionsFunctionName, entry2.AutomaticExecutionsFunctionName);
		EXPECT_EQ(entry1.AutomaticExecutionsNextBlockToCheck, entry2.AutomaticExecutionsNextBlockToCheck);
		EXPECT_EQ(entry1.AutomaticExecutionsPrepaidSince, entry2.AutomaticExecutionsPrepaidSince);
	}

	void AssertEqualServicePayments(const std::vector<state::ServicePayment>& entry1, const std::vector<state::ServicePayment>& entry2) {
		ASSERT_EQ(entry1.size(), entry2.size());
		for (auto i = 0u; i < entry1.size(); i++) {
			const auto& expectedPayment = entry1[i];
			const auto& actualPayment = entry2[i];
			EXPECT_EQ(expectedPayment.Amount, actualPayment.Amount);
			EXPECT_EQ(expectedPayment.MosaicId, actualPayment.MosaicId);
		}
	}

	void AssertEqualRequestedCalls(const std::deque<state::ContractCall>& entry1, const std::deque<state::ContractCall>& entry2) {
		ASSERT_EQ(entry1.size(), entry2.size());
		for (auto i = 0u; i < entry1.size(); i++) {
			const auto& expectedContractCall = entry1[i];
			const auto& actualContractCall = entry2[i];
			EXPECT_EQ(expectedContractCall.FileName, actualContractCall.FileName);
			EXPECT_EQ(expectedContractCall.ActualArguments, actualContractCall.ActualArguments);
			EXPECT_EQ(expectedContractCall.BlockHeight, actualContractCall.BlockHeight);
			EXPECT_EQ(expectedContractCall.CallId, actualContractCall.CallId);
			EXPECT_EQ(expectedContractCall.Caller, actualContractCall.Caller);
			EXPECT_EQ(expectedContractCall.DownloadCallPayment, actualContractCall.DownloadCallPayment);
			EXPECT_EQ(expectedContractCall.ExecutionCallPayment, actualContractCall.ExecutionCallPayment);
			EXPECT_EQ(expectedContractCall.FunctionName, actualContractCall.FunctionName);
			AssertEqualServicePayments(expectedContractCall.ServicePayments, actualContractCall.ServicePayments);
		}
	}

	void AssertEqualSuperContractData(const state::SuperContractEntry& entry1, const state::SuperContractEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		EXPECT_EQ(entry1.driveKey(), entry2.driveKey());
		EXPECT_EQ(entry1.executionPaymentKey(), entry2.executionPaymentKey());
		EXPECT_EQ(entry1.deploymentBaseModificationId(), entry2.deploymentBaseModificationId());
		EXPECT_EQ(entry1.assignee(), entry2.assignee());
		EXPECT_EQ(entry1.batches(), entry2.batches());
		EXPECT_EQ(entry1.deploymentStatus(), entry2.deploymentStatus());
		EXPECT_EQ(entry1.executorsInfo(), entry2.executorsInfo());
		EXPECT_EQ(entry1.nextBatchId(), entry2.nextBatchId());
		EXPECT_EQ(entry1.releasedTransactions(), entry2.releasedTransactions());
		AssertEqualAutomaticExecutionsInfo(entry1.automaticExecutionsInfo(), entry2.automaticExecutionsInfo());
		AssertEqualRequestedCalls(entry1.requestedCalls(), entry2.requestedCalls());
	}

	void AssertEqualDriveContract(const state::DriveContractEntry& entry1, const state::DriveContractEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		EXPECT_EQ(entry1.contractKey(), entry2.contractKey());
	}
}}
