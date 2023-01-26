/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExecutorEventHandler.h"
#include "catapult/model/EntityRange.h"

namespace catapult { namespace contract {

	ExecutorEventHandler::ExecutorEventHandler(
			const crypto::KeyPair& keyPair,
			TransactionSender&& transactionSender,
			TransactionStatusHandler& transactionHandler)
		: m_keyPair(keyPair)
		, m_transactionSender(std::move(transactionSender))
		, m_transactionStatusHandler(transactionHandler) {}

	void ExecutorEventHandler::endBatchTransactionIsReady(
			const sirius::contract::SuccessfulEndBatchExecutionTransactionInfo& transactionInfo) {
		auto transactionHash = m_transactionSender.sendSuccessfulEndBatchExecutionTransaction(transactionInfo);
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

}} // namespace catapult::contract