/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveDbrbProcessByNetworkBuilder.h"

namespace catapult { namespace builders {

	RemoveDbrbProcessByNetworkBuilder::RemoveDbrbProcessByNetworkBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
	{}

	void RemoveDbrbProcessByNetworkBuilder::setProcessId(const dbrb::ProcessId& id) {
		m_processId = id;
	}

	void RemoveDbrbProcessByNetworkBuilder::setTimestamp(const Timestamp& timestamp) {
		m_timestamp = timestamp;
	}

	void RemoveDbrbProcessByNetworkBuilder::addVote(model::Cosignature vote) {
		m_votes.emplace_back(std::move(vote));
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> RemoveDbrbProcessByNetworkBuilder::buildImpl() const {
		auto size = sizeof(TransactionType) + m_votes.size() * (Key_Size + Signature_Size);
		auto pTransaction = createTransaction<TransactionType>(size);
		pTransaction->ProcessId = m_processId;
		pTransaction->Timestamp = m_timestamp;
		pTransaction->VoteCount = utils::checked_cast<size_t, uint16_t>(m_votes.size());
		std::memcpy(static_cast<void*>(pTransaction->VotesPtr()), m_votes.data(), m_votes.size() * sizeof(model::Cosignature));
		return pTransaction;
	}

	model::UniqueEntityPtr<RemoveDbrbProcessByNetworkBuilder::Transaction> RemoveDbrbProcessByNetworkBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<RemoveDbrbProcessByNetworkBuilder::EmbeddedTransaction> RemoveDbrbProcessByNetworkBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
