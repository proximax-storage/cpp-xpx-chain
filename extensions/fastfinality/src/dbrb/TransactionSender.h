/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbConfiguration.h"
#include "BlockSubscriber.h"
#include "src/catapult/crypto/KeyPair.h"
#include "catapult/io/BlockChangeSubscriber.h"
#include "catapult/dbrb/Messages.h"
#include "catapult/extensions/ServerHooks.h"

namespace catapult { namespace harvesting { class UnlockedAccounts; }}

namespace catapult { namespace dbrb {

    class TransactionSender : public BlockHandler {
    public:
        void init(
				const crypto::KeyPair* pKeyPair,
				const config::ImmutableConfiguration& immutableConfig,
				const DbrbConfiguration& dbrbConfig,
				handlers::TransactionRangeHandler transactionRangeHandler,
				std::shared_ptr<harvesting::UnlockedAccounts> pHarvesterAccounts) {
			m_pKeyPair = pKeyPair;
			m_networkIdentifier = immutableConfig.NetworkIdentifier;
			m_generationHash = immutableConfig.GenerationHash;
			m_transactionTimeout = Timestamp(dbrbConfig.TransactionTimeout.millis());
			m_transactionRangeHandler = std::move(transactionRangeHandler);
			m_pHarvesterAccounts = std::move(pHarvesterAccounts);
		}

    public:
        void sendAddDbrbProcessTransaction();
		std::future<bool> sendRemoveDbrbProcessTransaction();
		void disableAddDbrbProcessTransaction();
		bool isAddDbrbProcessTransactionSent();
		void sendRemoveDbrbProcessByNetworkTransaction(const ProcessId& id, const Timestamp& timestamp, const std::map<ProcessId, Signature>& votes);
		bool isRemoveDbrbProcessByNetworkTransactionSent(const ProcessId& id);

	public:
		void handleBlock(const model::BlockElement& blockElement) override;

	private:
        void send(const std::shared_ptr<model::Transaction>& pTransaction);

    private:
		const crypto::KeyPair* m_pKeyPair;
        model::NetworkIdentifier m_networkIdentifier;
        GenerationHash m_generationHash;
		Timestamp m_transactionTimeout;
        handlers::TransactionRangeHandler m_transactionRangeHandler;
		std::shared_ptr<harvesting::UnlockedAccounts> m_pHarvesterAccounts;
		Hash256 m_addDbrbProcessTransactionHash;
		Timestamp m_addDbrbProcessTxTime;
		Hash256 m_removeDbrbProcessTransactionHash;
		std::shared_ptr<std::promise<bool>> m_pRemoveDbrbProcessTxConfirmedPromise;
		bool m_disableAddDbrbProcessTransaction = false;
		std::shared_mutex m_mutex;
		std::map<ProcessId, std::pair<Hash256, Timestamp>> m_removalsByNetwork;
    };
}}
