/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <utility>

#include <blockchain/SuccessfulEndBatchExecutionTransaction.h>
#include <blockchain/UnsuccessfulEndBatchExecutionTransaction.h>
#include <blockchain/EndBatchExecutionSingleTransaction.h>
#include <blockchain/SynchronizationSingleTransaction.h>
#include <blockchain/AggregatedTransaction.h>
#include "src/catapult/crypto/KeyPair.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/handlers/HandlerTypes.h"
#include "catapult/utils/NetworkTime.h"
#include "ExecutorConfiguration.h"

using namespace sirius::contract::blockchain;

namespace catapult { namespace contract {
	class TransactionSender {

	public:
		TransactionSender(
				const crypto::KeyPair& keyPair,
				const config::ImmutableConfiguration& immutableConfig,
				ExecutorConfiguration executorConfig,
				handlers::TransactionRangeHandler transactionRangeHandler);

	public:
		Hash256 sendSuccessfulEndBatchExecutionTransaction(const SuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		Hash256 sendUnsuccessfulEndBatchExecutionTransaction(const UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		Hash256 sendEndBatchExecutionSingleTransaction(const EndBatchExecutionSingleTransactionInfo& transactionInfo);
		Hash256 sendSynchronizationSingleTransaction(const SynchronizationSingleTransactionInfo& transactionInfo);
		Hash256 sendReleasedTransactions(const SerializedAggregatedTransaction& transaction);

	private:
		void send(std::shared_ptr<model::Transaction> pTransaction);

	private:
		const crypto::KeyPair& m_keyPair;
		model::NetworkIdentifier m_networkIdentifier;
		GenerationHash m_generationHash;
		ExecutorConfiguration m_executorConfig;
		handlers::TransactionRangeHandler m_transactionRangeHandler;
	};
}}
