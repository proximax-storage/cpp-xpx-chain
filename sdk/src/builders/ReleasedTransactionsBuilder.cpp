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

#include "ReleasedTransactionsBuilder.h"
#include "catapult/crypto/Signer.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/utils/Functional.h"

namespace catapult { namespace builders {

	using TransactionType = model::AggregateTransaction;

	ReleasedTransactionsBuilder::ReleasedTransactionsBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
	{}

	void ReleasedTransactionsBuilder::addTransaction(const std::vector<uint8_t>& payload) {
		m_transactions.push_back(payload);
	}

	model::UniqueEntityPtr<TransactionType> ReleasedTransactionsBuilder::build() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto payloadSize = utils::Sum(m_transactions, [](const auto& payload) { return payload.size(); });
		auto size = sizeof(TransactionType) + payloadSize;
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set transaction fields
		pTransaction->Type = model::Entity_Type_Aggregate_Complete;
		pTransaction->Version = model::MakeVersion(m_networkIdentifier, 4);
		pTransaction->PayloadSize = payloadSize;

		auto* pData = reinterpret_cast<uint8_t*>(pTransaction->TransactionsPtr());
		for (const auto& embeddedTransaction : m_transactions) {
			std::memcpy(pData, embeddedTransaction.data(), embeddedTransaction.size());
			pData += embeddedTransaction.size();
		}

		return pTransaction;
	}

	namespace {
		RawBuffer TransactionDataBuffer(const TransactionType& transaction) {
			return {
				reinterpret_cast<const uint8_t*>(&transaction) + model::VerifiableEntity::Header_Size,
				sizeof(TransactionType) - model::VerifiableEntity::Header_Size + transaction.PayloadSize
			};
		}
	}
}}
