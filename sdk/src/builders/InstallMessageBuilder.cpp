/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "InstallMessageBuilder.h"

namespace catapult { namespace builders {

	InstallMessageBuilder::InstallMessageBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer, dbrb::InstallMessage& message)
		: TransactionBuilder(networkIdentifier, signer)
		, m_message(message)
	{}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> InstallMessageBuilder::buildImpl() const {
		// Sequence size
		size_t payloadSize = sizeof(uint32_t);
		for (const auto& view : m_message.Sequence.data())
			payloadSize += sizeof(uint32_t) + view.Data.size() * (dbrb::ProcessId_Size + sizeof(dbrb::MembershipChange));
		// Certificate size
		payloadSize += sizeof(uint32_t) + m_message.ConvergedSignatures.size() * sizeof(model::Cosignature);

		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType) + payloadSize);
		pTransaction->MessageHash = dbrb::CalculateHash(m_message.toNetworkPacket(nullptr)->buffers());
		pTransaction->PayloadSize = utils::checked_cast<size_t, uint32_t>(payloadSize);

		auto pBuffer = pTransaction->PayloadPtr();
		dbrb::Write(pBuffer, m_message.Sequence);
		dbrb::Write(pBuffer, m_message.ConvergedSignatures);

		return pTransaction;
	}

	model::UniqueEntityPtr<InstallMessageBuilder::Transaction> InstallMessageBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<InstallMessageBuilder::EmbeddedTransaction> InstallMessageBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
