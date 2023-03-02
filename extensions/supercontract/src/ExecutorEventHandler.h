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
#include <executor/Executor.h>

using namespace sirius::contract::blockchain;

namespace catapult { namespace contract {
	class ExecutorEventHandler : public sirius::contract::ExecutorEventHandler {

	public:
		ExecutorEventHandler(
				const crypto::KeyPair& keyPair,
				TransactionSender&& transactionSender,
				std::shared_ptr<TransactionStatusHandler> pTransactionHandler);

	public:
		void endBatchTransactionIsReady(const SuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		void endBatchTransactionIsReady(const UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		void endBatchSingleTransactionIsReady(const EndBatchExecutionSingleTransactionInfo& transactionInfo);
		void synchronizationSingleTransactionIsReady(const SynchronizationSingleTransactionInfo& transactionInfo);
		void releasedTransactionsAreReady(const SerializedAggregatedTransaction& transaction) override;

		void setExecutor(std::weak_ptr<sirius::contract::Executor> pExecutor);

	private:

		const crypto::KeyPair& m_keyPair;
		TransactionSender m_transactionSender;
		std::shared_ptr<TransactionStatusHandler> m_pTransactionStatusHandler;
		std::weak_ptr<sirius::contract::Executor> m_pExecutor;
	};
}}