/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "executor/ExecutorEventHandler.h"
#include "src/catapult/crypto/KeyPair.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/handlers/HandlerTypes.h"
#include "catapult/utils/NetworkTime.h"
#include "ExecutorConfiguration.h"
#include "TransactionSender.h"
#include "TransactionStatusHandler.h"

namespace catapult { namespace contract {
	class ExecutorEventHandler : public sirius::contract::ExecutorEventHandler {

	public:
		ExecutorEventHandler(
				const crypto::KeyPair& keyPair,
				TransactionSender&& transactionSender,
				TransactionStatusHandler& transactionHandler);

	public:
		void endBatchTransactionIsReady(const sirius::contract::SuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		void endBatchTransactionIsReady(const sirius::contract::UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		void endBatchSingleTransactionIsReady(const sirius::contract::EndBatchExecutionSingleTransactionInfo& transactionInfo);
		void synchronizationSingleTransactionIsReady(const sirius::contract::SynchronizationSingleTransactionInfo& transactionInfo);
		void releasedTransactionsAreReady(const std::vector<std::vector<uint8_t>>& payloads) override;

	private:

		const crypto::KeyPair& m_keyPair;
		TransactionSender m_transactionSender;
		TransactionStatusHandler& m_transactionStatusHandler;
	};
}}