/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <utility>

#include <executor/Transactions.h>
#include "src/catapult/crypto/KeyPair.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/handlers/HandlerTypes.h"
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace contract {
	class TransactionSender {

	public:
		TransactionSender(
				const crypto::KeyPair& keyPair,
				const config::ImmutableConfiguration& immutableConfig,
				handlers::TransactionRangeHandler transactionRangeHandler)
			: m_keyPair(keyPair)
			, m_networkIdentifier(immutableConfig.NetworkIdentifier)
			, m_generationHash(immutableConfig.GenerationHash)
			, m_transactionRangeHandler(std::move(transactionRangeHandler))
		{}

	public:
		Hash256 sendSuccessfulEndBatchExecutionTransaction(const sirius::contract::SuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		Hash256 sendUnsuccessfulEndBatchExecutionTransaction(const sirius::contract::UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		Hash256 sendEndBatchExecutionSingleTransaction(const sirius::contract::EndBatchExecutionSingleTransactionInfo& transactionInfo);

	private:
		void send(std::shared_ptr<model::Transaction> pTransaction);

	private:
		const crypto::KeyPair& m_keyPair;
		model::NetworkIdentifier m_networkIdentifier;
		GenerationHash m_generationHash;
		handlers::TransactionRangeHandler m_transactionRangeHandler;
	};
}}
