/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ContractStateImpl.h"
#include "src/cache/SuperContractCache.h"
#include "src/cache/DriveContractCache.h"
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

	std::optional<Height> ContractStateImpl::getAutomaticExecutionsEnabledSince(const Key& contractKey) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		auto contractIt = pSuperContractCacheView->find(contractKey);

		// The caller must be sure that the contract exists
		const state::SuperContractEntry& contractEntry = contractIt.get();
		return contractEntry.automaticExecutionsInfo().AutomaticExecutionsEnabledSince;
	}

	Hash256 ContractStateImpl::getDriveState(const Key& contractKey) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		auto contractIt = pSuperContractCacheView->find(contractKey);

		// The caller must be sure that the contract exists
		const auto& contractEntry = contractIt.get();

		m_pDriveStateBrowser->getDriveState(m_pCache->createView().toReadOnly(), contractEntry.driveKey());
	}

	std::set<Key> ContractStateImpl::getContracts(const Key& executorKey) const {
		auto readOnlyCache = m_pCache->createView().toReadOnly();
		auto drives = m_pDriveStateBrowser->getDrives(readOnlyCache, executorKey);
		auto pDriveContractCacheView = getCacheView<cache::DriveContractCache>();
		std::set<Key> contracts;
		for (const auto& drive: drives) {
			auto driveContractIt = pDriveContractCacheView->find(drive);
			const auto& pDriveContractEntry = driveContractIt.tryGet();
			if (pDriveContractEntry) {
				contracts.insert(pDriveContractEntry->contractKey());
			}
		}
		return contracts;
	}

	std::set<Key> ContractStateImpl::getExecutors(const Key& contractKey) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		auto contractIt = pSuperContractCacheView->find(contractKey);

		// The caller must be sure that the contract exists
		const auto& contractEntry = contractIt.get();

		return m_pDriveStateBrowser->getReplicators(m_pCache->createView().toReadOnly(), contractEntry.driveKey());
	}

	ContractInfo ContractStateImpl::getContractInfo(const Key& contractKey) const {
		auto pSuperContractCacheView = getCacheView<cache::SuperContractCache>();
		auto contractIt = pSuperContractCacheView->find(contractKey);

		// The caller must be sure that the contract exists
		const auto& contractEntry = contractIt.get();

		ContractInfo contractInfo;
		contractInfo.DriveKey = contractEntry.driveKey();
		contractInfo.Executors = getExecutors(contractKey);
		contractInfo.BatchesExecuted = contractEntry.nextBatchId();

		const auto& automaticExecutionInfo = contractEntry.automaticExecutionsInfo();
		contractInfo.AutomaticExecutionsFileName = automaticExecutionInfo.AutomaticExecutionFileName;
		contractInfo.AutomaticExecutionsFunctionName = automaticExecutionInfo.AutomaticExecutionsFunctionName;
		contractInfo.AutomaticExecutionCallPayment = automaticExecutionInfo.AutomaticExecutionCallPayment;
		contractInfo.AutomaticDownloadCallPayment = automaticExecutionInfo.AutomaticDownloadCallPayment;
		contractInfo.AutomaticExecutionsEnabledSince = automaticExecutionInfo.AutomaticExecutionsEnabledSince;
		contractInfo.AutomaticExecutionsNextBlockToCheck = automaticExecutionInfo.AutomaticExecutionsNextBlockToCheck;
		if (!contractEntry.batches().empty()) {
			const auto& batch = contractEntry.batches().back();
			PublishedBatchInfo lastBatch;
			lastBatch.BatchIndex = contractEntry.batches().size();
			lastBatch.BatchSuccess = batch.Success;
			lastBatch.DriveState = getDriveState(contractKey);
			lastBatch.PoExVerificationInfo = lastBatch.PoExVerificationInfo;
			for (const auto& [key, executor]: contractEntry.executorsInfo()) {
				if (executor.NextBatchToApprove == contractEntry.nextBatchId()) {
					lastBatch.Cosigners.insert(key);
				}
			}
			contractInfo.LastPublishedBatch = std::move(lastBatch);
		}
		for (const auto& call: contractEntry.requestedCalls()) {
			ManualCallInfo callInfo;
			callInfo.CallId = call.CallId;
			callInfo.FileName = call.FileName;
			callInfo.FunctionName = call.FunctionName;
			callInfo.Arguments = call.ActualArguments;
			callInfo.ExecutionPayment = call.ExecutionCallPayment;
			callInfo.DownloadPayment = call.DownloadCallPayment;
			callInfo.Caller = call.Caller;
			callInfo.BlockHeight = call.BlockHeight;
			contractInfo.ManualCalls.push_back(std::move(callInfo));
		}
	}
}}
