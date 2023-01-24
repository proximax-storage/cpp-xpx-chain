/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include <utility>

#include <executor/Transactions.h>
#include "src/catapult/crypto/KeyPair.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/handlers/HandlerTypes.h"
#include "catapult/utils/NetworkTime.h"
#include "ExecutorConfiguration.h"

namespace catapult { namespace contract {
	class TransactionSender {

	public:
		TransactionSender(
				const crypto::KeyPair& keyPair,
				const config::ImmutableConfiguration& immutableConfig,
				ExecutorConfiguration executorConfig,
				handlers::TransactionRangeHandler transactionRangeHandler);

	public:
		Hash256 sendSuccessfulEndBatchExecutionTransaction(const sirius::contract::SuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		Hash256 sendUnsuccessfulEndBatchExecutionTransaction(const sirius::contract::UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		Hash256 sendEndBatchExecutionSingleTransaction(const sirius::contract::EndBatchExecutionSingleTransactionInfo& transactionInfo);
		Hash256 sendReleasedTransactions(const std::vector<std::vector<uint8_t>>& payloads);

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
