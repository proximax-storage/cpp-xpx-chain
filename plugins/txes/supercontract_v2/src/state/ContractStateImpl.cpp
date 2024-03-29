/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractStateImpl.h"
#include "src/cache/SuperContractCache.h"
#include "src/cache/DriveContractCache.h"
#include "src/utils/ContractUtils.h"
#include <catapult/cache/ReadOnlyCatapultCache.h>

namespace catapult { namespace state {

	ContractStateImpl::ContractStateImpl(const std::unique_ptr<DriveStateBrowser>& driveStateBrowser)
		: m_pDriveStateBrowser(driveStateBrowser) {}

	bool ContractStateImpl::contractExists(const Key& contractKey) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		return pSuperContractCacheView->contains(contractKey);
	}

	std::shared_ptr<const model::BlockElement> ContractStateImpl::getBlock(Height height) const {
		if (!m_blockProvider) {
			CATAPULT_THROW_RUNTIME_ERROR("block provider not set");
		}

		return m_blockProvider(height);
	}

	std::optional<Height> ContractStateImpl::getAutomaticExecutionsEnabledSince(
			const Key& contractKey,
			const Height& actualHeight,
			const config::BlockchainConfiguration config) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		auto contractIt = pSuperContractCacheView->find(contractKey);

		// The caller must be sure that the contract exists
		const state::SuperContractEntry& contractEntry = contractIt.get();
		return utils::automaticExecutionsEnabledSince(contractEntry, actualHeight, config);
	}

	Hash256 ContractStateImpl::getDriveState(const Key& contractKey) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		auto contractIt = pSuperContractCacheView->find(contractKey);

		// The caller must be sure that the contract exists
		const auto& contractEntry = contractIt.get();

		auto view = m_pCache->createView();
		return m_pDriveStateBrowser->getDriveState(view.toReadOnly(), contractEntry.driveKey());
	}

	std::set<Key> ContractStateImpl::getContracts(const Key& executorKey) const {
		auto view = m_pCache->createView();
		auto readOnlyCache = view.toReadOnly();
		auto drives = m_pDriveStateBrowser->getDrives(readOnlyCache, executorKey);
		auto pDriveContractCacheView = getCacheView<cache::DriveContractCache>();
		std::set<Key> contracts;
		for (const auto& drive : drives) {
			auto driveContractIt = pDriveContractCacheView->find(drive);
			const auto* pDriveContractEntry = driveContractIt.tryGet();
			if (pDriveContractEntry) {
				contracts.insert(pDriveContractEntry->contractKey());
			}
		}
		return contracts;
	}

	std::map<Key, ExecutorStateInfo> ContractStateImpl::getExecutors(const Key& contractKey) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		auto contractIt = pSuperContractCacheView->find(contractKey);

		// The caller must be sure that the contract exists
		const auto& contractEntry = contractIt.get();

		auto view = m_pCache->createView();
		auto executorKeys = m_pDriveStateBrowser->getReplicators(view.toReadOnly(), contractEntry.driveKey());

		std::map<Key, ExecutorStateInfo> executors;

		const auto& executorsInfo = contractEntry.executorsInfo();
		for (const auto& key : executorKeys) {
			ExecutorStateInfo digest;
			auto it = executorsInfo.find(key);
			if (it != executorsInfo.end()) {
				const auto& info = it->second;
				digest.NextBatchToApprove = info.NextBatchToApprove;
				digest.PoEx.StartBatchId = info.PoEx.StartBatchId;
				digest.PoEx.T = info.PoEx.T;
				digest.PoEx.R = info.PoEx.R;
			} else {
				digest.NextBatchToApprove = contractEntry.nextBatchId();
				digest.PoEx.StartBatchId = contractEntry.nextBatchId();
			}
			executors[key] = digest;
		}
		return executors;
	}

	ContractInfo ContractStateImpl::getContractInfo(
			const Key& contractKey,
			const Height& actualHeight,
			const config::BlockchainConfiguration config) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		auto contractIt = pSuperContractCacheView->find(contractKey);

		// The caller must be sure that the contract exists
		const auto& contractEntry = contractIt.get();

		ContractInfo contractInfo;
		contractInfo.DriveKey = contractEntry.driveKey();
		contractInfo.Executors = getExecutors(contractKey);
		contractInfo.DeploymentBaseModificationId = contractEntry.deploymentBaseModificationId();

		for (const auto& [batchId, batch]: contractEntry.batches()) {
			contractInfo.RecentBatches[batchId] = batch.PoExVerificationInformation;
		}

		const auto& automaticExecutionsInfo = contractEntry.automaticExecutionsInfo();
		contractInfo.AutomaticExecutionsFileName = automaticExecutionsInfo.AutomaticExecutionsFileName;
		contractInfo.AutomaticExecutionsFunctionName = automaticExecutionsInfo.AutomaticExecutionsFunctionName;
		contractInfo.AutomaticExecutionCallPayment = automaticExecutionsInfo.AutomaticExecutionCallPayment;
		contractInfo.AutomaticDownloadCallPayment = automaticExecutionsInfo.AutomaticDownloadCallPayment;
		contractInfo.AutomaticExecutionsEnabledSince = utils::automaticExecutionsEnabledSince(contractEntry, actualHeight, config);
		contractInfo.AutomaticExecutionsNextBlockToCheck = automaticExecutionsInfo.AutomaticExecutionsNextBlockToCheck;
		if (!contractEntry.batches().empty()) {
			auto batchIt = --contractEntry.batches().end();
			const auto& batch = batchIt->second;
			PublishedBatchInfo lastBatch;
			lastBatch.BatchIndex = batchIt->first;
			lastBatch.BatchSuccess = batch.Success;
			lastBatch.DriveState = getDriveState(contractKey);
			lastBatch.PoExVerificationInfo = lastBatch.PoExVerificationInfo;
			for (const auto& [key, executor] : contractEntry.executorsInfo()) {
				if (executor.NextBatchToApprove == contractEntry.nextBatchId()) {
					lastBatch.Cosigners.insert(key);
				}
			}
			contractInfo.LastPublishedBatch = std::move(lastBatch);
		}
		for (const auto& call : contractEntry.requestedCalls()) {
			ManualCallInfo callInfo;
			callInfo.CallId = call.CallId;
			callInfo.FileName = call.FileName;
			callInfo.FunctionName = call.FunctionName;
			callInfo.Arguments = call.ActualArguments;
			callInfo.ExecutionPayment = call.ExecutionCallPayment;
			callInfo.DownloadPayment = call.DownloadCallPayment;
			callInfo.Caller = call.Caller;
			callInfo.BlockHeight = call.BlockHeight;
			for (const auto& payment : call.ServicePayments) {
				callInfo.Payments.push_back({ payment.MosaicId, payment.Amount });
			}
			contractInfo.ManualCalls.push_back(std::move(callInfo));
		}

		return contractInfo;
	}
	Height ContractStateImpl::getAutomaticExecutionsNextBlockToCheck(const Key& contractKey) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		auto contractIt = pSuperContractCacheView->find(contractKey);

		// The caller must be sure that the contract exists
		const state::SuperContractEntry& contractEntry = contractIt.get();

		return contractEntry.automaticExecutionsInfo().AutomaticExecutionsNextBlockToCheck;
	}
}} // namespace catapult::state
