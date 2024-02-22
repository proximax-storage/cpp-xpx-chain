/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionSender.h"
#include "sdk/src/builders/AddDbrbProcessBuilder.h"
#include "sdk/src/builders/RemoveDbrbProcessBuilder.h"
#include "sdk/src/builders/RemoveDbrbProcessByNetworkBuilder.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/harvesting_core/UnlockedAccounts.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/utils/NetworkTime.h"

namespace catapult { namespace dbrb {

    void TransactionSender::sendAddDbrbProcessTransaction() {
		std::lock_guard<std::mutex> guard(m_mutex);
		if (m_disableAddDbrbProcessTransaction) {
			CATAPULT_LOG(debug) << "[DBRB] skipping node registration in the DBRB system because the node has requested to be removed";
			return;
		}

		auto now = utils::NetworkTime();
		if (m_addDbrbProcessTransactionHash != Hash256() && now < m_addDbrbProcessTxTime + m_transactionTimeout) {
			CATAPULT_LOG(debug) << "[DBRB] skipping node registration in the DBRB system because of previous transaction";
			return;
		}

		m_addDbrbProcessTxTime = now;
		builders::AddDbrbProcessBuilder builder(m_networkIdentifier, m_pKeyPair->publicKey());
		for (const auto& keyPair : m_pHarvesterAccounts->view())
			builder.addHarvesterKey(keyPair.publicKey());
		auto pTransaction = utils::UniqueToShared(builder.build());
		pTransaction->Deadline = now + m_transactionTimeout;
		send(pTransaction);
		m_addDbrbProcessTransactionHash = model::CalculateHash(*pTransaction, m_generationHash);
		CATAPULT_LOG(debug) << "[DBRB] sent AddDbrbProcessTransaction " << m_addDbrbProcessTransactionHash;
    }

	std::future<bool> TransactionSender::sendRemoveDbrbProcessTransaction() {
		builders::RemoveDbrbProcessBuilder builder(m_networkIdentifier, m_pKeyPair->publicKey());
		for (const auto& keyPair : m_pHarvesterAccounts->view())
			builder.addHarvesterKey(keyPair.publicKey());
		auto pTransaction = utils::UniqueToShared(builder.build());
		pTransaction->Deadline = utils::NetworkTime() + m_transactionTimeout;
		send(pTransaction);

		{
			std::lock_guard<std::mutex> guard(m_mutex);
			m_removeDbrbProcessTransactionHash = model::CalculateHash(*pTransaction, m_generationHash);
			CATAPULT_LOG(debug) << "[DBRB] sent RemoveDbrbProcessTransaction " << m_removeDbrbProcessTransactionHash;

			m_pRemoveDbrbProcessTxConfirmedPromise = std::make_shared<std::promise<bool>>();
			return m_pRemoveDbrbProcessTxConfirmedPromise->get_future();
		}
    }

	void TransactionSender::disableAddDbrbProcessTransaction() {
		std::lock_guard<std::mutex> guard(m_mutex);
		m_disableAddDbrbProcessTransaction = true;
	}

	bool TransactionSender::isAddDbrbProcessTransactionSent() {
		std::lock_guard<std::mutex> guard(m_mutex);
		return (m_addDbrbProcessTransactionHash != Hash256());
	}

	bool TransactionSender::isRemoveDbrbProcessByNetworkTransactionSent(const ProcessId& id) {
		std::lock_guard<std::mutex> guard(m_mutex);
		auto iter = m_removalsByNetwork.find(id);
		return (iter != m_removalsByNetwork.cend() && utils::NetworkTime() < iter->second.second + m_transactionTimeout);
	}

    void TransactionSender::send(const std::shared_ptr<model::Transaction>& pTransaction) {
		extensions::TransactionExtensions(m_generationHash).sign(*m_pKeyPair, *pTransaction);
        auto range = model::TransactionRange::FromEntity(pTransaction);
        m_transactionRangeHandler({ std::move(range), m_pKeyPair->publicKey() });
    }

	void TransactionSender::handleBlock(const model::BlockElement& blockElement) {
		std::lock_guard<std::mutex> guard(m_mutex);

		if (m_addDbrbProcessTransactionHash == Hash256() && m_removeDbrbProcessTransactionHash == Hash256())
			return;

		for (const auto& transactionElement : blockElement.Transactions) {
			if (m_addDbrbProcessTransactionHash == transactionElement.EntityHash) {
				CATAPULT_LOG(debug) << "[DBRB] AddDbrbProcessTransaction " << m_addDbrbProcessTransactionHash << " confirmed";
				m_addDbrbProcessTxTime = Timestamp(0);
				m_addDbrbProcessTransactionHash = Hash256();
			} else if (m_removeDbrbProcessTransactionHash == transactionElement.EntityHash) {
				m_removeDbrbProcessTransactionHash = Hash256();
				CATAPULT_LOG(debug) << "[DBRB] RemoveDbrbProcessTransaction " << m_removeDbrbProcessTransactionHash << " confirmed";
				if (m_pRemoveDbrbProcessTxConfirmedPromise) {
					m_pRemoveDbrbProcessTxConfirmedPromise->set_value(true);
					m_pRemoveDbrbProcessTxConfirmedPromise = nullptr;
				}
			} else {
				for (auto iter = m_removalsByNetwork.begin(); iter != m_removalsByNetwork.end(); ++iter) {
					if (iter->second.first == transactionElement.EntityHash) {
						m_removalsByNetwork.erase(iter);
						break;
					}
				}
			}
		}
	}

    void TransactionSender::sendRemoveDbrbProcessByNetworkTransaction(const ProcessId& id, const Timestamp& timestamp, const std::map<ProcessId, Signature>& votes) {
		std::lock_guard<std::mutex> guard(m_mutex);
		auto now = utils::NetworkTime();
		auto iter = m_removalsByNetwork.find(id);
		if (iter != m_removalsByNetwork.cend() && now < iter->second.second + m_transactionTimeout) {
			CATAPULT_LOG(debug) << "[DBRB] skipping removing " << id << " from DBRB system because of previous transaction";
			return;
		}

		builders::RemoveDbrbProcessByNetworkBuilder builder(m_networkIdentifier, m_pKeyPair->publicKey());
		builder.setProcessId(id);
		builder.setTimestamp(timestamp);
		for (const auto& [id, signature] : votes)
			builder.addVote({ id, signature });

		auto pTransaction = utils::UniqueToShared(builder.build());
		pTransaction->Deadline = now + m_transactionTimeout;
		send(pTransaction);
		auto hash = model::CalculateHash(*pTransaction, m_generationHash);
		CATAPULT_LOG(debug) << "[DBRB] sent RemoveDbrbProcessByNetworkTransaction " << hash;
		m_removalsByNetwork[id] = std::make_pair(hash, now);
    }
}}
