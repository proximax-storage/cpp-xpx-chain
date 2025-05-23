/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "SecretProofBuilder.h"

namespace catapult { namespace builders {

	SecretProofBuilder::SecretProofBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
			, m_hashAlgorithm()
			, m_secret()
			, m_recipient()
			, m_proof()
	{}

	void SecretProofBuilder::setHashAlgorithm(model::LockHashAlgorithm hashAlgorithm) {
		m_hashAlgorithm = hashAlgorithm;
	}

	void SecretProofBuilder::setSecret(const Hash256& secret) {
		m_secret = secret;
	}

	void SecretProofBuilder::setRecipient(const UnresolvedAddress& recipient) {
		m_recipient = recipient;
	}

	void SecretProofBuilder::setProof(const RawBuffer& proof) {
		if (0 == proof.Size)
			CATAPULT_THROW_INVALID_ARGUMENT("argument `proof` cannot be empty");

		if (!m_proof.empty())
			CATAPULT_THROW_RUNTIME_ERROR("`proof` field already set");

		m_proof.resize(proof.Size);
		m_proof.assign(proof.pData, proof.pData + proof.Size);
	}

	model::UniqueEntityPtr<SecretProofBuilder::Transaction> SecretProofBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<SecretProofBuilder::EmbeddedTransaction> SecretProofBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> SecretProofBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto size = sizeof(TransactionType);
		size += m_proof.size();
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set fixed transaction fields
		pTransaction->HashAlgorithm = m_hashAlgorithm;
		pTransaction->Secret = m_secret;
		pTransaction->Recipient = m_recipient;
		pTransaction->ProofSize = utils::checked_cast<size_t, uint16_t>(m_proof.size());

		// 3. set transaction attachments
		std::copy(m_proof.cbegin(), m_proof.cend(), pTransaction->ProofPtr());

		return pTransaction;
	}
}}
