/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

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

}}