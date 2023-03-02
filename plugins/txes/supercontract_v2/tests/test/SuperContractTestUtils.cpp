/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <gtest/gtest.h>
#include "SuperContractTestUtils.h"

namespace catapult { namespace test {

	state::SuperContractEntry CreateSuperContractEntry(Key key) {
		state::SuperContractEntry entry(key);
		entry.setDriveKey( test::GenerateRandomByteArray<Key>() );
		entry.setExecutionPaymentKey( test::GenerateRandomByteArray<Key>() );
		entry.setAssignee( test::GenerateRandomByteArray<Key>() );
		entry.setDeploymentBaseModificationId( test::GenerateRandomByteArray<Hash256>() );

		// poex
		crypto::Scalar scalar(std::array<uint8_t ,32>{5});
		crypto::CurvePoint curvePoint = crypto::CurvePoint::BasePoint() * scalar;

		// automatic executions info
		entry.automaticExecutionsInfo().AutomaticExecutionFileName = "aaa";
		entry.automaticExecutionsInfo().AutomaticExecutionsFunctionName = "aaa";
		entry.automaticExecutionsInfo().AutomaticExecutionsNextBlockToCheck = Height(1);
		entry.automaticExecutionsInfo().AutomaticExecutionCallPayment = Amount(1);
		entry.automaticExecutionsInfo().AutomaticDownloadCallPayment = Amount(1);
		entry.automaticExecutionsInfo().AutomatedExecutionsNumber = 0U;

		// contract call
		state::ServicePayment servicePayment;
		servicePayment.Amount = Amount(10);
		servicePayment.MosaicId = UnresolvedMosaicId(1);
		state::ContractCall contractCall;
		contractCall.CallId = test::GenerateRandomByteArray<Hash256>();
		contractCall.Caller = test::GenerateRandomByteArray<Key>();
		contractCall.FileName = "aaa";
		contractCall.FunctionName = "aaa";
		contractCall.ActualArguments = "aaa";
		contractCall.ExecutionCallPayment = Amount(10);
		contractCall.DownloadCallPayment = Amount(10);
		contractCall.BlockHeight = Height(10);
		contractCall.ServicePayments.push_back(servicePayment);  // service payment x3
		contractCall.ServicePayments.push_back(servicePayment);
		contractCall.ServicePayments.push_back(servicePayment);
		entry.requestedCalls().push_back(contractCall);  // contract call x3
		entry.requestedCalls().push_back(contractCall);
		entry.requestedCalls().push_back(contractCall);

		// executors info
		Key executor1 = test::GenerateRandomByteArray<Key>();
		Key executor2 = test::GenerateRandomByteArray<Key>();
		Key executor3 = test::GenerateRandomByteArray<Key>();
		state::ExecutorInfo executorInfo;
		executorInfo.PoEx.R = scalar;
		executorInfo.PoEx.T = curvePoint;
		entry.executorsInfo()[executor1] = executorInfo;  // executors info x3
		entry.executorsInfo()[executor2] = executorInfo;
		entry.executorsInfo()[executor3] = executorInfo;

		// batches
		state::CompletedCall completedCall;
		completedCall.CallId = test::GenerateRandomByteArray<Hash256>();
		completedCall.Caller = test::GenerateRandomByteArray<Key>();
		completedCall.Status = 0;
		completedCall.DownloadWork = Amount(10);
		completedCall.ExecutionWork = Amount(10);
		state::Batch batch;
		batch.Success = false;
		batch.PoExVerificationInformation = curvePoint;
		batch.CompletedCalls.push_back(completedCall);  // completed call x3
		batch.CompletedCalls.push_back(completedCall);
		batch.CompletedCalls.push_back(completedCall);
		entry.batches()[1] = batch;  // batch x3
		entry.batches()[2] = batch;
		entry.batches()[3] = batch;

		// released transactions
		Hash256 hash1 = test::GenerateRandomByteArray<Hash256>();
		Hash256 hash2 = test::GenerateRandomByteArray<Hash256>();
		Hash256 hash3 = test::GenerateRandomByteArray<Hash256>();
		entry.releasedTransactions().emplace(hash1);  // released transaction x3
		entry.releasedTransactions().emplace(hash2);
		entry.releasedTransactions().emplace(hash3);
		return entry;
	}

	state::DriveContractEntry CreateDriveContractEntry(Key key) {
		state::DriveContractEntry entry(key);
		return entry;
	}

	void AssertEqualExecutorsInfo(const std::map<Key, state::ExecutorInfo>& expectedExecutorInfo, const std::map<Key, state::ExecutorInfo>& executorInfo){
		ASSERT_EQ(expectedExecutorInfo.size(), executorInfo.size());
		for (const auto& pair : executorInfo) {
			const auto expIter = expectedExecutorInfo.find(pair.first);
			ASSERT_NE(expIter, expectedExecutorInfo.end());
			EXPECT_EQ(expIter->second.NextBatchToApprove, pair.second.NextBatchToApprove);
			EXPECT_EQ(expIter->second.PoEx.StartBatchId, pair.second.PoEx.StartBatchId);
			EXPECT_EQ(expIter->second.PoEx.T, pair.second.PoEx.T);
			EXPECT_EQ(expIter->second.PoEx.R, pair.second.PoEx.R);
		}
	}

	void AssertEqualBatches(const std::map<uint64_t , state::Batch>& expectedBatch, const std::map<uint64_t , state::Batch>& batch) {
		ASSERT_EQ(expectedBatch.size(), batch.size());
		for (const auto& pair : batch) {
			const auto expIter = expectedBatch.find(pair.first);
			ASSERT_NE(expIter, expectedBatch.end());
			EXPECT_EQ(expIter->second.Success, pair.second.Success);
			EXPECT_EQ(expIter->second.PoExVerificationInformation, pair.second.PoExVerificationInformation);
			for (int i = 0; i < expIter->second.CompletedCalls.size(); i++) {
				EXPECT_EQ(expIter->second.CompletedCalls.at(i).CallId, pair.second.CompletedCalls.at(i).CallId);
				EXPECT_EQ(expIter->second.CompletedCalls.at(i).Caller, pair.second.CompletedCalls.at(i).Caller);
				EXPECT_EQ(expIter->second.CompletedCalls.at(i).Status, pair.second.CompletedCalls.at(i).Status);
				EXPECT_EQ(
						expIter->second.CompletedCalls.at(i).ExecutionWork,
						pair.second.CompletedCalls.at(i).ExecutionWork);
				EXPECT_EQ(
						expIter->second.CompletedCalls.at(i).DownloadWork,
						pair.second.CompletedCalls.at(i).DownloadWork);
			}
		}
	}

	void AssertEqualSupercontractData(const state::SuperContractEntry& expectedEntry, const state::SuperContractEntry& entry){
		EXPECT_EQ(expectedEntry.key(), entry.key());
		EXPECT_EQ(expectedEntry.driveKey(), entry.driveKey());
		EXPECT_EQ(expectedEntry.executionPaymentKey(), entry.executionPaymentKey());
		EXPECT_EQ(expectedEntry.assignee(), entry.assignee());
		EXPECT_EQ(expectedEntry.deploymentBaseModificationId(), entry.deploymentBaseModificationId());
		// automatic executions info
		EXPECT_EQ(expectedEntry.automaticExecutionsInfo().AutomaticExecutionFileName, entry.automaticExecutionsInfo().AutomaticExecutionFileName);
		EXPECT_EQ(expectedEntry.automaticExecutionsInfo().AutomaticExecutionsFunctionName, entry.automaticExecutionsInfo().AutomaticExecutionsFunctionName);
		EXPECT_EQ(expectedEntry.automaticExecutionsInfo().AutomaticExecutionsNextBlockToCheck, entry.automaticExecutionsInfo().AutomaticExecutionsNextBlockToCheck);
		EXPECT_EQ(expectedEntry.automaticExecutionsInfo().AutomaticExecutionCallPayment, entry.automaticExecutionsInfo().AutomaticExecutionCallPayment);
		EXPECT_EQ(expectedEntry.automaticExecutionsInfo().AutomaticDownloadCallPayment, entry.automaticExecutionsInfo().AutomaticDownloadCallPayment);
		EXPECT_EQ(expectedEntry.automaticExecutionsInfo().AutomatedExecutionsNumber, entry.automaticExecutionsInfo().AutomatedExecutionsNumber);
		EXPECT_EQ(expectedEntry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince, entry.automaticExecutionsInfo().AutomaticExecutionsPrepaidSince);
		// contract call
		for (int i=0; i<expectedEntry.requestedCalls().size(); i++){
			EXPECT_EQ(expectedEntry.requestedCalls().at(i).CallId, entry.requestedCalls().at(i).CallId);
			EXPECT_EQ(expectedEntry.requestedCalls().at(i).Caller, entry.requestedCalls().at(i).Caller);
			EXPECT_EQ(expectedEntry.requestedCalls().at(i).FileName, entry.requestedCalls().at(i).FileName);
			EXPECT_EQ(expectedEntry.requestedCalls().at(i).FunctionName, entry.requestedCalls().at(i).FunctionName);
			EXPECT_EQ(expectedEntry.requestedCalls().at(i).ActualArguments, entry.requestedCalls().at(i).ActualArguments);
			EXPECT_EQ(expectedEntry.requestedCalls().at(i).ExecutionCallPayment, entry.requestedCalls().at(i).ExecutionCallPayment);
			EXPECT_EQ(expectedEntry.requestedCalls().at(i).DownloadCallPayment, entry.requestedCalls().at(i).DownloadCallPayment);
			EXPECT_EQ(expectedEntry.requestedCalls().at(i).BlockHeight, entry.requestedCalls().at(i).BlockHeight);
			// service payment
			for (int j=0; j<expectedEntry.requestedCalls().at(i).ServicePayments.size(); j++){
				EXPECT_EQ(expectedEntry.requestedCalls().at(i).ServicePayments.at(j).MosaicId, entry.requestedCalls().at(i).ServicePayments.at(j).MosaicId);
				EXPECT_EQ(expectedEntry.requestedCalls().at(i).ServicePayments.at(j).Amount, entry.requestedCalls().at(i).ServicePayments.at(j).Amount);
			}
		}
		// executors info
		EXPECT_EQ(expectedEntry.executorsInfo().size(), entry.executorsInfo().size());
		AssertEqualExecutorsInfo(expectedEntry.executorsInfo(), entry.executorsInfo());
		// batches
		EXPECT_EQ(expectedEntry.batches().size(), entry.batches().size());
		AssertEqualBatches(expectedEntry.batches(), entry.batches());
		// released transaction
		EXPECT_EQ(expectedEntry.releasedTransactions().size(), entry.releasedTransactions().size());
		for (const auto& it : entry.releasedTransactions()) {
			const auto expIter = expectedEntry.releasedTransactions().find(it);
			ASSERT_NE(expIter, expectedEntry.releasedTransactions().end());
		}
	}
}}