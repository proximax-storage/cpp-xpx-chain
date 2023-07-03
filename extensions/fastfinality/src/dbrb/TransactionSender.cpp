/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionSender.h"
#include "sdk/src/builders/AddDbrbProcessBuilder.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/model/EntityHasher.h"
#include <boost/dynamic_bitset.hpp>

namespace catapult { namespace dbrb {

    Hash256 TransactionSender::sendAddDbrbProcessTransaction() {
        builders::AddDbrbProcessBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
		for (const auto& keyPair : m_pHarvesterAccounts->view())
			builder.addHarvesterKey(keyPair.publicKey());
        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + m_transactionTimeout;
        send(pTransaction);

        return model::CalculateHash(*pTransaction, m_generationHash);
    }

    void TransactionSender::send(const std::shared_ptr<model::Transaction>& pTransaction) {
		extensions::TransactionExtensions(m_generationHash).sign(m_keyPair, *pTransaction);
        auto range = model::TransactionRange::FromEntity(pTransaction);
        m_transactionRangeHandler({std::move(range), m_keyPair.publicKey()});
    }

	Timestamp TransactionSender::transactionTimeout() {
		return m_transactionTimeout;
	}
}}
