/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/dbrb/Messages.h"
#include "DbrbConfiguration.h"
#include "src/catapult/crypto/KeyPair.h"
#include "catapult/utils/NetworkTime.h"
#include "catapult/extensions/ServerHooks.h"

namespace catapult { namespace dbrb {

    class TransactionSender {
    public:
        TransactionSender(
				const crypto::KeyPair& keyPair,
				const config::ImmutableConfiguration& immutableConfig,
				const DbrbConfiguration& dbrbConfig,
				handlers::TransactionRangeHandler transactionRangeHandler)
			: m_keyPair(keyPair)
			, m_networkIdentifier(immutableConfig.NetworkIdentifier)
			, m_generationHash(immutableConfig.GenerationHash)
			, m_transactionTimeout(dbrbConfig.TransactionTimeout.millis())
			, m_transactionRangeHandler(std::move(transactionRangeHandler))
		{}

    public:
        Hash256 sendAddDbrbProcessTransaction();
		Timestamp transactionTimeout();

	private:
        void send(const std::shared_ptr<model::Transaction>& pTransaction);

    private:
		const crypto::KeyPair& m_keyPair;
        model::NetworkIdentifier m_networkIdentifier;
        GenerationHash m_generationHash;
		Timestamp m_transactionTimeout;
        handlers::TransactionRangeHandler m_transactionRangeHandler;
    };
}}
