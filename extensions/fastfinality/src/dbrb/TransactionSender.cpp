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
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace dbrb {

    void TransactionSender::sendAddDbrbProcessTransaction() {
		auto now = utils::NetworkTime();
		if (m_transactionHash == Hash256() || now >= m_lastRegistrationTxTime + m_transactionTimeout) {
			m_lastRegistrationTxTime = now;

			builders::AddDbrbProcessBuilder builder(m_networkIdentifier, m_pKeyPair->publicKey());
			for (const auto& keyPair : m_pHarvesterAccounts->view())
				builder.addHarvesterKey(keyPair.publicKey());
			auto pTransaction = utils::UniqueToShared(builder.build());
			pTransaction->Deadline = utils::NetworkTime() + m_transactionTimeout;
			send(pTransaction);

			m_transactionHash = model::CalculateHash(*pTransaction, m_generationHash);
			CATAPULT_LOG(debug) << "[DBRB] sent AddDbrbProcessTransaction " << m_transactionHash;
		} else {
			CATAPULT_LOG(debug) << "[DBRB] skipping node registration in the DBRB system because of previous transaction";
		}
    }

    void TransactionSender::send(const std::shared_ptr<model::Transaction>& pTransaction) {
		extensions::TransactionExtensions(m_generationHash).sign(*m_pKeyPair, *pTransaction);
        auto range = model::TransactionRange::FromEntity(pTransaction);
        m_transactionRangeHandler({ std::move(range), m_pKeyPair->publicKey() });
    }

	void TransactionSender::handleBlock(const model::BlockElement& blockElement) {
		if (m_transactionHash == Hash256())
			return;

		for (const auto& transactionElement : blockElement.Transactions) {
			if (m_transactionHash == transactionElement.EntityHash) {
				m_lastRegistrationTxTime = Timestamp(0);
				m_transactionHash = Hash256();
			}
		}
	}
}}
