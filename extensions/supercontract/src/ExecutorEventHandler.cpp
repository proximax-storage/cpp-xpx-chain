/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExecutorEventHandler.h"
#include "catapult/model/EntityRange.h"
#include "plugins/txes/supercontract_v2/src/validators/Results.h"

namespace catapult::contract {

	namespace {

		void handleFailedEndBatchExecution(
				uint32_t status,
				const std::weak_ptr<sirius::contract::Executor>& pExecutorWeak,
				const sirius::contract::ContractKey& contractKey,
				uint64_t batchId,
				bool batchSuccess) {
			auto pExecutor = pExecutorWeak.lock();
			if (!pExecutor) {
				return;
			}

			auto validationResult = validators::ValidationResult(status);

			std::set<validators::ValidationResult> handledResults = {
				validators::Failure_SuperContract_Invalid_Start_Batch_Id,
				validators::Failure_SuperContract_Invalid_Batch_Proof,
				validators::Failure_SuperContract_Not_Enough_Signatures,
				validators::Failure_SuperContract_Is_Not_Executor
			};

			if (handledResults.find(validationResult) != handledResults.end()) {
				try {
					pExecutor->onEndBatchExecutionFailed(FailedEndBatchExecutionTransactionInfo {
							contractKey, batchId, batchSuccess });
				}
				catch (...) {}
			}
		}

	} // namespace

	ExecutorEventHandler::ExecutorEventHandler(
			const crypto::KeyPair& keyPair,
			TransactionSender&& transactionSender,
			std::shared_ptr<TransactionStatusHandler> pTransactionHandler)
		: m_keyPair(keyPair)
		, m_transactionSender(std::move(transactionSender))
		, m_pTransactionStatusHandler(std::move(pTransactionHandler)) {}

	void ExecutorEventHandler::endBatchTransactionIsReady(
			const SuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {
		auto transactionHash = m_transactionSender.sendSuccessfulEndBatchExecutionTransaction(transactionInfo);

		m_pTransactionStatusHandler->addHandler(
				transactionHash,
				[contractKey = transactionInfo.m_contractKey,
				 batchId = transactionInfo.m_batchIndex,
				 pExecutorWeak = m_pExecutor](uint32_t status) {
					handleFailedEndBatchExecution(status, pExecutorWeak, contractKey, batchId, true);
				});
	}

	void ExecutorEventHandler::endBatchTransactionIsReady(
			const UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {
		auto transactionHash = m_transactionSender.sendUnsuccessfulEndBatchExecutionTransaction(transactionInfo);
		m_pTransactionStatusHandler->addHandler(
				transactionHash,
				[contractKey = transactionInfo.m_contractKey,
				 batchId = transactionInfo.m_batchIndex,
				 pExecutorWeak = m_pExecutor](uint32_t status) {
					handleFailedEndBatchExecution(status, pExecutorWeak, contractKey, batchId, false);
				});
	}

	void ExecutorEventHandler::endBatchSingleTransactionIsReady(
			const EndBatchExecutionSingleTransactionInfo& transactionInfo) {
		auto transactionHash = m_transactionSender.sendEndBatchExecutionSingleTransaction(transactionInfo);
	}

	void ExecutorEventHandler::synchronizationSingleTransactionIsReady(
			const SynchronizationSingleTransactionInfo& transactionInfo) {
		auto transactionHash = m_transactionSender.sendSynchronizationSingleTransaction(transactionInfo);
	}

	void ExecutorEventHandler::releasedTransactionsAreReady(const sirius::contract::blockchain::SerializedAggregatedTransaction& transaction) {
		auto transactionHash = m_transactionSender.sendReleasedTransactions(transaction);
	}

	void ExecutorEventHandler::setExecutor(std::weak_ptr<sirius::contract::Executor> pExecutor) {
		m_pExecutor = std::move(pExecutor);
	}
} // namespace catapult::contract