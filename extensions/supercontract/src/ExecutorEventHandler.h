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

namespace catapult { namespace contract {
	class ExecutorEventHandler : public sirius::contract::ExecutorEventHandler {

	public:
		ExecutorEventHandler(
				const crypto::KeyPair& keyPair,
				const config::ImmutableConfiguration& immutableConfig,
				ExecutorConfiguration executorConfig,
				handlers::TransactionRangeHandler transactionRangeHandler)
			: m_keyPair(keyPair)
			, m_networkIdentifier(immutableConfig.NetworkIdentifier)
			, m_executorConfig(std::move(executorConfig))
			, m_generationHash(immutableConfig.GenerationHash)
		{}

	public:
		void endBatchTransactionIsReady(const sirius::contract::SuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		void endBatchTransactionIsReady(const sirius::contract::UnsuccessfulEndBatchExecutionTransactionInfo& transactionInfo);
		void endBatchSingleTransactionIsReady(const sirius::contract::EndBatchExecutionSingleTransactionInfo& transactionInfo);
		void synchronizationSingleTransactionIsReady(const sirius::contract::SynchronizationSingleTransactionInfo& transactionInfo);

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