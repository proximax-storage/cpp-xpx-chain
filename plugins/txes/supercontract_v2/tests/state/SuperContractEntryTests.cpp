/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/state/SuperContractEntry.h"
#include "tests/TestHarness.h"

namespace catapult { namespace state {

#define TEST_CLASS SuperContractEntryTests

	TEST(TEST_CLASS, CanCreateSuperContractEntry) {
		// Act:
		auto key = test::GenerateRandomByteArray<Key>();
		auto entry = SuperContractEntry(key);

		// Assert:
		EXPECT_EQ(key, entry.key());
	}

	TEST(TEST_CLASS, CanAccessDriveKey) {
		// Arrange:
		auto driveKey = test::GenerateRandomByteArray<Key>();
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Key(), entry.driveKey());

		// Act:
		entry.setDriveKey(driveKey);

		// Assert:
		EXPECT_EQ(driveKey, entry.driveKey());
	}

	TEST(TEST_CLASS, CanAccessDeploymentBaseModificationId) {
		// Arrange:
		auto rootHash = test::GenerateRandomByteArray<Hash256>();
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(Hash256(), entry.deploymentBaseModificationId());

		// Act:
		entry.setDeploymentBaseModificationId(rootHash);

		// Assert:
		EXPECT_EQ(rootHash, entry.deploymentBaseModificationId());
	}

	TEST(TEST_CLASS, CanAccessContractCall) {
		// Arrange:
		std::deque<ContractCall> requestedCalls;
		auto folderNameBytes = test::GenerateRandomDataVector<uint8_t>(512);
		ContractCall contractCall;
		contractCall.CallId = test::GenerateRandomByteArray<Hash256>();;
		contractCall.Caller = test::GenerateRandomByteArray<Key>();;
		contractCall.FileName = std::string(folderNameBytes.begin(), folderNameBytes.end());
		contractCall.FunctionName = std::string(folderNameBytes.begin(), folderNameBytes.end());
		contractCall.ActualArguments = std::string(folderNameBytes.begin(), folderNameBytes.end());;
		contractCall.ExecutionCallPayment = Amount(50);
		contractCall.DownloadCallPayment = Amount(50);
		contractCall.ServicePayments.emplace_back(ServicePayment{UnresolvedMosaicId(test::Random()), Amount(50)});
		contractCall.BlockHeight = Height(test::Random());
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(requestedCalls.size(), entry.requestedCalls().size());


		// Act:
		entry.requestedCalls().emplace_back(contractCall);

		// Assert:
		EXPECT_EQ(contractCall.CallId, entry.requestedCalls().begin()->CallId);
		EXPECT_EQ(contractCall.Caller, entry.requestedCalls().begin()->Caller);
		EXPECT_EQ(contractCall.FileName, entry.requestedCalls().begin()->FileName);
		EXPECT_EQ(contractCall.FunctionName, entry.requestedCalls().begin()->FunctionName);
		EXPECT_EQ(contractCall.ActualArguments, entry.requestedCalls().begin()->ActualArguments);
		EXPECT_EQ(contractCall.ExecutionCallPayment, entry.requestedCalls().begin()->ExecutionCallPayment);
		EXPECT_EQ(contractCall.DownloadCallPayment, entry.requestedCalls().begin()->DownloadCallPayment);
		EXPECT_EQ(contractCall.ServicePayments.begin()->MosaicId, entry.requestedCalls().begin()->ServicePayments.begin()->MosaicId);
		EXPECT_EQ(contractCall.ServicePayments.begin()->Amount, entry.requestedCalls().begin()->ServicePayments.begin()->Amount);
		EXPECT_EQ(contractCall.BlockHeight, entry.requestedCalls().begin()->BlockHeight);
	}

	TEST(TEST_CLASS, CanAccessExecutorsInfo) {
		// Arrange:
		std::map<Key, ExecutorInfo> executorsInfo;
		ExecutorInfo executorInfo;
		crypto::Scalar scalar(std::array<uint8_t ,32>{5});
		crypto::CurvePoint curvePoint = crypto::CurvePoint::BasePoint() * scalar;
		Key key = test::GenerateRandomByteArray<Key>();
		executorInfo.PoEx.R = scalar;
		executorInfo.PoEx.T = curvePoint;
		executorsInfo[key] = executorInfo;
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(0, entry.executorsInfo().size());

		// Act:
		entry.executorsInfo() = executorsInfo;

		// Assert:
		EXPECT_EQ(executorsInfo.begin()->first, entry.executorsInfo().begin()->first);
		EXPECT_EQ(executorsInfo.begin()->second.NextBatchToApprove, entry.executorsInfo().begin()->second.NextBatchToApprove);
		EXPECT_EQ(executorsInfo.begin()->second.PoEx.StartBatchId, entry.executorsInfo().begin()->second.PoEx.StartBatchId);
		EXPECT_EQ(executorsInfo.begin()->second.PoEx.R, entry.executorsInfo().begin()->second.PoEx.R);
		EXPECT_EQ(executorsInfo.begin()->second.PoEx.T, entry.executorsInfo().begin()->second.PoEx.T);
	}

	TEST(TEST_CLASS, CanAccessBatches) {
		// Arrange:
		std::map<uint64_t, Batch> batches;
		Batch batch;
		crypto::Scalar scalar(std::array<uint8_t ,32>{5});
		crypto::CurvePoint curvePoint = crypto::CurvePoint::BasePoint() * scalar;
		batch.PoExVerificationInformation = curvePoint;
		CompletedCall completedCall;
		completedCall.CallId = test::GenerateRandomByteArray<Hash256>();
		completedCall.Caller = test::GenerateRandomByteArray<Key>();
		completedCall.ExecutionWork = Amount(100);
		completedCall.DownloadWork = Amount(100);
		batch.CompletedCalls.emplace_back(completedCall);
		batches[1] = batch;
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(0, entry.batches().size());

		// Act:
		entry.batches() = batches;

		// Assert:
		EXPECT_EQ(batches.begin()->first, entry.batches().begin()->first);
		EXPECT_EQ(batches.begin()->second.Success, entry.batches().begin()->second.Success);
		EXPECT_EQ(batches.begin()->second.PoExVerificationInformation, entry.batches().begin()->second.PoExVerificationInformation);
		EXPECT_EQ(batches.begin()->second.CompletedCalls.begin()->CallId, entry.batches().begin()->second.CompletedCalls.begin()->CallId);
		EXPECT_EQ(batches.begin()->second.CompletedCalls.begin()->Caller, entry.batches().begin()->second.CompletedCalls.begin()->Caller);
		EXPECT_EQ(batches.begin()->second.CompletedCalls.begin()->Status, entry.batches().begin()->second.CompletedCalls.begin()->Status);
		EXPECT_EQ(batches.begin()->second.CompletedCalls.begin()->DownloadWork, entry.batches().begin()->second.CompletedCalls.begin()->DownloadWork);
		EXPECT_EQ(batches.begin()->second.CompletedCalls.begin()->ExecutionWork, entry.batches().begin()->second.CompletedCalls.begin()->ExecutionWork);
	}

	TEST(TEST_CLASS, CanAccessReleasedTransaction) {
		// Arrange:
		std::multiset<Hash256> releasedTransactions;
		auto hash = test::GenerateRandomByteArray<Hash256>();
		releasedTransactions.emplace(hash);
		auto entry = SuperContractEntry(Key());

		// Sanity:
		ASSERT_EQ(0, entry.releasedTransactions().size());

		// Act:
		entry.releasedTransactions().emplace(hash);

		// Assert:
		EXPECT_EQ(*releasedTransactions.begin(), *entry.releasedTransactions().begin());
	}
}}