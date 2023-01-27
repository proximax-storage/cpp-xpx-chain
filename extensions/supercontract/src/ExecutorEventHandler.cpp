/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExecutorEventHandler.h"
#include "catapult/model/EntityRange.h"

namespace catapult::contract {

		ExecutorEventHandler::ExecutorEventHandler(
				const crypto::KeyPair& keyPair,
				TransactionSender&& transactionSender,
				std::shared_ptr<TransactionStatusHandler> pTransactionHandler)
			: m_keyPair(keyPair)
			, m_transactionSender(std::move(transactionSender))
			, m_pTransactionStatusHandler(std::move(pTransactionHandler)) {}

		void ExecutorEventHandler::endBatchTransactionIsReady(
				const sirius::contract::SuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {
			auto transactionHash = m_transactionSender.sendSuccessfulEndBatchExecutionTransaction(transactionInfo);

//			m_transactionStatusHandler.addHandler(
//					transactionHash,
//					[blockHash = info.m_blockHash,
//					 downloadChannelId = info.m_downloadChannelId,
//					 pReplicatorWeak = m_pReplicator](uint32_t status) {
//						auto pReplicator = pReplicatorWeak.lock();
//						if (!pReplicator)
//							return;
//
//						auto validationResult = validators::ValidationResult(status);
//						CATAPULT_LOG(debug) << "download approval transaction completed with " << validationResult;
//
//						std::set<validators::ValidationResult> handledResults = {
//							validators::Failure_Storage_Signature_Count_Insufficient,
//							validators::Failure_Storage_Opinion_Invalid_Key
//						};
//
//						if (handledResults.find(validationResult) != handledResults.end())
//							pReplicator->asyncDownloadApprovalTransactionHasFailedInvalidOpinions(
//									blockHash, downloadChannelId);
//					});
//		}
	}

	void ExecutorEventHandler::endBatchTransactionIsReady(
			const sirius::contract::UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {
		auto transactionHash = m_transactionSender.sendUnsuccessfulEndBatchExecutionTransaction(transactionInfo);
	}

	void ExecutorEventHandler::endBatchSingleTransactionIsReady(
			const sirius::contract::EndBatchExecutionSingleTransactionInfo& transactionInfo) {
		auto transactionHash = m_transactionSender.sendEndBatchExecutionSingleTransaction(transactionInfo);
	}

	void ExecutorEventHandler::synchronizationSingleTransactionIsReady(
			const sirius::contract::SynchronizationSingleTransactionInfo& transactionInfo) {
		auto transactionHash = m_transactionSender.sendSynchronizationSingleTransaction(transactionInfo);
	}

	void ExecutorEventHandler::releasedTransactionsAreReady(const std::vector<std::vector<uint8_t>>& payloads) {
		auto transactionHash = m_transactionSender.sendReleasedTransactions(payloads);
	}

	void ExecutorEventHandler::setExecutor(std::weak_ptr<sirius::contract::Executor> pExecutor) {
		m_pExecutor = std::move(pExecutor);
	}

}