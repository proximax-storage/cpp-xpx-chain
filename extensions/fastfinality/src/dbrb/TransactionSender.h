/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Messages.h"
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
				DbrbConfiguration dbrbConfig,
				handlers::TransactionRangeHandler transactionRangeHandler)
			: m_keyPair(keyPair)
			, m_networkIdentifier(immutableConfig.NetworkIdentifier)
			, m_generationHash(immutableConfig.GenerationHash)
			, m_dbrbConfig(std::move(dbrbConfig))
			, m_transactionRangeHandler(std::move(transactionRangeHandler))
		{}

    public:
        Hash256 sendInstallMessageTransaction(InstallMessage& message);

	private:
        void send(std::shared_ptr<model::Transaction> pTransaction);

    private:
		const crypto::KeyPair& m_keyPair;
        model::NetworkIdentifier m_networkIdentifier;
        GenerationHash m_generationHash;
		DbrbConfiguration m_dbrbConfig;
        handlers::TransactionRangeHandler m_transactionRangeHandler;
    };
}}
