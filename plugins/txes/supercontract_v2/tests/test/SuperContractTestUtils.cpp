/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <gtest/gtest.h>
#include "SuperContractTestUtils.h"
#include <catapult/cache/ReadOnlyCatapultCache.h>

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

	void AssertEqualExecutorsInfo(const std::map<Key, state::ExecutorInfo>& entry1, const std::map<Key, state::ExecutorInfo>& entry2) {
		ASSERT_EQ(entry1.size(), entry2.size());
		for (const auto& it : entry1){
			auto actualEntry = entry2.find(it.first);
			ASSERT_TRUE(actualEntry != entry2.end());

			EXPECT_EQ(it.second.NextBatchToApprove, actualEntry->second.NextBatchToApprove);
			EXPECT_EQ(it.second.PoEx.R, actualEntry->second.PoEx.R);
			EXPECT_EQ(it.second.PoEx.T, actualEntry->second.PoEx.T);
			EXPECT_EQ(it.second.PoEx.StartBatchId, actualEntry->second.PoEx.StartBatchId);
		}
	}

	void AssertEqualBatches(const std::map<uint64_t, state::Batch>& entry1, const std::map<uint64_t, state::Batch>& entry2) {
		ASSERT_EQ(entry1.size(), entry2.size());
		for (const auto& it : entry1) {
			auto actualEntry = entry2.find(it.first);
			ASSERT_TRUE(actualEntry != entry2.end());

			EXPECT_EQ(it.second.Success, actualEntry->second.Success);
			EXPECT_EQ(it.second.PoExVerificationInformation, actualEntry->second.PoExVerificationInformation);

			ASSERT_EQ(it.second.CompletedCalls.size(), actualEntry->second.CompletedCalls.size());
			for (auto i = 0u; i < entry1.size(); i++) {
				const auto& expectedCall = it.second.CompletedCalls[i];
				const auto& actualCall = actualEntry->second.CompletedCalls[i];
				EXPECT_EQ(expectedCall.CallId, actualCall.CallId);
				EXPECT_EQ(expectedCall.Caller, actualCall.Caller);
				EXPECT_EQ(expectedCall.DownloadWork, actualCall.DownloadWork);
				EXPECT_EQ(expectedCall.ExecutionWork, actualCall.ExecutionWork);
				EXPECT_EQ(expectedCall.Status, actualCall.Status);
			}
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
		EXPECT_EQ(entry1.deploymentStatus(), entry2.deploymentStatus());
		EXPECT_EQ(entry1.nextBatchId(), entry2.nextBatchId());
		EXPECT_EQ(entry1.releasedTransactions(), entry2.releasedTransactions());
		AssertEqualBatches(entry1.batches(), entry2.batches());
		AssertEqualExecutorsInfo(entry1.executorsInfo(), entry2.executorsInfo());
		AssertEqualRequestedCalls(entry1.requestedCalls(), entry2.requestedCalls());
		AssertEqualAutomaticExecutionsInfo(entry1.automaticExecutionsInfo(), entry2.automaticExecutionsInfo());
	}

	void AssertEqualDriveContract(const state::DriveContractEntry& entry1, const state::DriveContractEntry& entry2) {
		EXPECT_EQ(entry1.key(), entry2.key());
		EXPECT_EQ(entry1.contractKey(), entry2.contractKey());
	}

	uint16_t DriveStateBrowserImpl::getOrderedReplicatorsCount(
			const catapult::cache::ReadOnlyCatapultCache& cache,
			const catapult::Key& driveKey) const {
//		const auto& driveCache = cache.template sub<cache::BcDriveCache>();
//		auto driveIter = driveCache.find(driveKey);
//		const auto& driveEntry = driveIter.get();
//		return driveEntry.replicatorCount();

		return {};
	}

	Key DriveStateBrowserImpl::getDriveOwner(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const {
//		const auto& driveCache = cache.template sub<cache::BcDriveCache>();
//		auto driveIter = driveCache.find(driveKey);
//		const auto& driveEntry = driveIter.get();
//		return driveEntry.owner();

		return {};
	}

	std::set<Key> DriveStateBrowserImpl::getReplicators(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const {
//		const auto& driveCache = cache.template sub<cache::BcDriveCache>();
//		auto driveIter = driveCache.find(driveKey);
//		const auto& driveEntry = driveIter.get();
//		return driveEntry.replicators();

		return {};
	}

	std::set<Key> DriveStateBrowserImpl::getDrives(const cache::ReadOnlyCatapultCache &cache, const Key &replicatorKey) const {
//		const auto& replicatorCache = cache.template sub<cache::ReplicatorCache>();
//		auto replicatorIt = replicatorCache.find(replicatorKey);
//		auto* pReplicatorEntry = replicatorIt.tryGet();
//		if (!pReplicatorEntry) {
//			return {};
//		}
//		std::set<Key> drives;
//		for (const auto& [key, _]: pReplicatorEntry->drives()) {
//			drives.insert(key);
//		}
//		return drives;

		return {};
	}

	Hash256 DriveStateBrowserImpl::getDriveState(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey) const {
//		const auto& driveCache = cache.template sub<cache::BcDriveCache>();
//		auto driveIter = driveCache.find(driveKey);
//		const auto& driveEntry = driveIter.get();
//		return driveEntry.rootHash();

		return {};
	}

	Hash256 DriveStateBrowserImpl::getLastModificationId(const cache::ReadOnlyCatapultCache& cache, const Key& driveKey)
	const {
//		const auto& driveCache = cache.template sub<cache::BcDriveCache>();
//		auto driveIter = driveCache.find(driveKey);
//		const auto& driveEntry = driveIter.get();
//		return driveEntry.lastModificationId();

		return {};
	}
}}