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

#pragma once
#include "catapult/model/NetworkInfo.h"
#include "catapult/model/Transaction.h"
#include "catapult/utils/Casting.h"
#include "catapult/utils/MemoryUtils.h"
#include "catapult/functions.h"

namespace catapult { namespace builders {

	/// Base transaction builder.
	class TransactionBuilder {
	public:
		/// Creates a transaction builder with \a networkIdentifier and \a signer.
		TransactionBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
				: m_networkIdentifier(networkIdentifier)
				, m_signer(signer)
		{}

	public:
		/// Returns signer.
		const Key& signer() const {
			return m_signer;
		}

	public:
		/// Sets transaction \a deadline.
		void setDeadline(catapult::Timestamp deadline) {
			m_deadline = deadline;
		}

		/// Sets maximum transaction \a fee.
		void setMaxFee(catapult::Amount fee) {
			m_maxFee = fee;
		}

	private:
		void setAdditionalFields(model::EmbeddedTransaction&) const
		{}

		void setAdditionalFields(model::Transaction& transaction) const {
			transaction.Deadline = m_deadline;
			transaction.MaxFee = m_maxFee;
		}

	protected:
		template<typename TTransaction>
		std::unique_ptr<TTransaction> createTransaction(size_t size) const {
			auto pTransaction = utils::MakeUniqueWithSize<TTransaction>(size);
			std::memset(static_cast<void*>(pTransaction.get()), 0, sizeof(TTransaction));

			// verifiable entity data
			pTransaction->Size = utils::checked_cast<size_t, uint32_t>(size);
			pTransaction->Type = TTransaction::Entity_Type;
			pTransaction->Version = MakeVersion(m_networkIdentifier, TTransaction::Current_Version);
			pTransaction->Signer = m_signer;

			// transaction data
			setAdditionalFields(*pTransaction);
			return pTransaction;
		}

		template<typename T, typename Predicate>
		static void InsertSorted(std::vector<T>& vector, const T& element, Predicate orderPredicate) {
			auto iter = std::upper_bound(vector.begin(), vector.end(), element, orderPredicate);
			if (iter != vector.begin() && !orderPredicate(*(iter - 1), element) && !orderPredicate(element, *(iter - 1)))
				CATAPULT_THROW_RUNTIME_ERROR("duplicate element in sorted set");

			vector.insert(iter, element);
		}

	private:
		const model::NetworkIdentifier m_networkIdentifier;
		const Key& m_signer;

		Timestamp m_deadline;
		Amount m_maxFee;
	};
}}
