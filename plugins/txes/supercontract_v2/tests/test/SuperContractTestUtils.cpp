/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuperContractTestUtils.h"
#include <catapult/cache/ReadOnlyCatapultCache.h>

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
