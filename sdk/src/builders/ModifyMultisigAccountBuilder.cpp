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

#include "ModifyMultisigAccountBuilder.h"

namespace catapult { namespace builders {

	using Modification = model::CosignatoryModification;

	template<typename TRegularTransaction, typename TEmbeddedTransaction>
	ModifyMultisigAccountBuilderT<TRegularTransaction, TEmbeddedTransaction>::ModifyMultisigAccountBuilderT(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
			, m_minRemovalDelta(0)
			, m_minApprovalDelta(0)
	{}

	template<typename TRegularTransaction, typename TEmbeddedTransaction>
	void ModifyMultisigAccountBuilderT<TRegularTransaction, TEmbeddedTransaction>::setMinRemovalDelta(int8_t minRemovalDelta) {
		m_minRemovalDelta = minRemovalDelta;
	}

	template<typename TRegularTransaction, typename TEmbeddedTransaction>
	void ModifyMultisigAccountBuilderT<TRegularTransaction, TEmbeddedTransaction>::setMinApprovalDelta(int8_t minApprovalDelta) {
		m_minApprovalDelta = minApprovalDelta;
	}

	template<typename TRegularTransaction, typename TEmbeddedTransaction>
	void ModifyMultisigAccountBuilderT<TRegularTransaction, TEmbeddedTransaction>::addCosignatoryModification(model::CosignatoryModificationType type, const Key& key) {
		m_modifications.push_back(Modification{ type, key });
	}

	template<typename TRegularTransaction, typename TEmbeddedTransaction>
	template<typename TransactionType>
	std::unique_ptr<TransactionType> ModifyMultisigAccountBuilderT<TRegularTransaction, TEmbeddedTransaction>::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto size = sizeof(TransactionType) + m_modifications.size() * sizeof(Modification);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set transaction fields
		pTransaction->MinRemovalDelta = m_minRemovalDelta;
		pTransaction->MinApprovalDelta = m_minApprovalDelta;

		// 3. set sizes upfront, so that pointers are calculated correctly
		pTransaction->ModificationsCount = utils::checked_cast<size_t, uint8_t>(m_modifications.size());

		// 4. set modifications
		if (!m_modifications.empty()) {
			auto* pModification = pTransaction->ModificationsPtr();
			for (const auto& modification : m_modifications) {
				*pModification = modification;
				++pModification;
			}
		}

		return pTransaction;
	}

	template<typename TRegularTransaction, typename TEmbeddedTransaction>
	std::unique_ptr<TRegularTransaction> ModifyMultisigAccountBuilderT<TRegularTransaction, TEmbeddedTransaction>::build() const {
		return buildImpl<TRegularTransaction>();
	}

	template<typename TRegularTransaction, typename TEmbeddedTransaction>
	std::unique_ptr<TEmbeddedTransaction> ModifyMultisigAccountBuilderT<TRegularTransaction, TEmbeddedTransaction>::buildEmbedded() const {
		return buildImpl<TEmbeddedTransaction>();
	}

	template class ModifyMultisigAccountBuilderT<
		model::ModifyMultisigAccountTransaction,
		model::EmbeddedModifyMultisigAccountTransaction>;

	template class ModifyMultisigAccountBuilderT<
		model::ModifyMultisigAccountAndReputationTransaction,
		model::EmbeddedModifyMultisigAccountAndReputationTransaction>;
}}
