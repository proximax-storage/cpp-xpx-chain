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

#include "catapult/crypto/Signature.h"
#include "AggregateTransactionBuilder.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/utils/Functional.h"

namespace catapult { namespace builders {

	template<typename TDescriptor>
	using TransactionType = model::AggregateTransaction<TDescriptor>;

	template<typename TDescriptor>
	AggregateTransactionBuilder<TDescriptor>::AggregateTransactionBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
	{}
	template<typename TDescriptor>
	void AggregateTransactionBuilder<TDescriptor>::addTransaction(AggregateTransactionBuilder::EmbeddedTransactionPointer&& pTransaction) {
		m_pTransactions.push_back(std::move(pTransaction));
	}
	template<typename TDescriptor>
	model::UniqueEntityPtr<TransactionType<TDescriptor>> AggregateTransactionBuilder<TDescriptor>::build() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto payloadSize = utils::Sum(m_pTransactions, [](const auto& pEmbeddedTransaction) { return pEmbeddedTransaction->Size; });
		auto size = sizeof(TransactionType<TDescriptor>) + payloadSize;
		auto pTransaction = createTransaction<TransactionType<TDescriptor>>(size);

		// 2. set transaction fields
		if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>)
			pTransaction->Type = model::Entity_Type_Aggregate_Bonded_V1;
		else
			pTransaction->Type = model::Entity_Type_Aggregate_Bonded_V2;

		pTransaction->PayloadSize = payloadSize;

		auto* pData = reinterpret_cast<uint8_t*>(pTransaction->TransactionsPtr());
		for (const auto& pEmbeddedTransaction : m_pTransactions) {
			std::memcpy(pData, pEmbeddedTransaction.get(), pEmbeddedTransaction->Size);
			pData += pEmbeddedTransaction->Size;
		}

		return pTransaction;
	}

	template class AggregateTransactionBuilder<model::AggregateTransactionRawDescriptor>;
	template class AggregateTransactionBuilder<model::AggregateTransactionExtendedDescriptor>;

	namespace {
		template<typename TDescriptor>
		RawBuffer TransactionDataBuffer(const TransactionType<TDescriptor>& transaction) {
			return {
				reinterpret_cast<const uint8_t*>(&transaction) + model::VerifiableEntity::Header_Size,
				sizeof(TransactionType<TDescriptor>) - model::VerifiableEntity::Header_Size + transaction.PayloadSize
			};
		}
	}

	template<typename TDescriptor>
	AggregateCosignatureAppender<TDescriptor>::AggregateCosignatureAppender(
			const GenerationHash& generationHash,
			model::UniqueEntityPtr<TransactionType<TDescriptor>>&& pAggregateTransaction)
			: m_generationHash(generationHash)
			, m_pAggregateTransaction(std::move(pAggregateTransaction))
	{}

	template<typename TDescriptor>
	void AggregateCosignatureAppender<TDescriptor>::cosign(const crypto::KeyPair& cosigner) {
		if (m_cosignatures.empty()) {
			if constexpr(std::is_same_v<TDescriptor, model::AggregateTransactionRawDescriptor>)
				m_pAggregateTransaction->Type = model::Entity_Type_Aggregate_Complete_V1;
			else
				m_pAggregateTransaction->Type = model::Entity_Type_Aggregate_Complete_V2;
			m_transactionHash = model::CalculateHash(
					*m_pAggregateTransaction,
					m_generationHash,
					TransactionDataBuffer(*m_pAggregateTransaction));
		}

		typename TDescriptor::CosignatureType cosignature{ cosigner.publicKey(), {} };
		crypto::SignatureFeatureSolver::Sign(cosigner, m_transactionHash, cosignature.Signature);
		m_cosignatures.push_back(cosignature);
	}

	template<typename TDescriptor>
	model::UniqueEntityPtr<TransactionType<TDescriptor>> AggregateCosignatureAppender<TDescriptor>::build() const {
		auto cosignaturesSize = sizeof(typename TDescriptor::CosignatureType) * m_cosignatures.size();
		auto size = m_pAggregateTransaction->Size + static_cast<uint32_t>(cosignaturesSize);
		auto pTransaction = utils::MakeUniqueWithSize<TransactionType<TDescriptor>>(size);

		std::memcpy(static_cast<void*>(pTransaction.get()), m_pAggregateTransaction.get(), m_pAggregateTransaction->Size);
		pTransaction->Size = size;
		std::memcpy(static_cast<void*>(pTransaction->CosignaturesPtr()), m_cosignatures.data(), cosignaturesSize);
		return pTransaction;
	}

	template class AggregateCosignatureAppender<model::AggregateTransactionRawDescriptor>;
	template class AggregateCosignatureAppender<model::AggregateTransactionExtendedDescriptor>;
}}
