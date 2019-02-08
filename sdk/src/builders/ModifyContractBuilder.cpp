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

#include "ModifyContractBuilder.h"

namespace catapult { namespace builders {

	ModifyContractBuilder::ModifyContractBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
	{}

	void ModifyContractBuilder::setDurationDelta(const int64_t& durationDelta) {
		m_durationDelta = durationDelta;
	}

	void ModifyContractBuilder::setHash(const Hash256& hash) {
		m_hash = hash;
	}

	void ModifyContractBuilder::addCustomerModification(model::CosignatoryModificationType type, const Key& key) {
		m_customerModifications.push_back(model::CosignatoryModification{ type, key });
	}

	void ModifyContractBuilder::addExecutorModification(model::CosignatoryModificationType type, const Key& key) {
		m_executorModifications.push_back(model::CosignatoryModification{ type, key });
	}

	void ModifyContractBuilder::addVerifierModification(model::CosignatoryModificationType type, const Key& key) {
		m_verifierModifications.push_back(model::CosignatoryModification{ type, key });
	}

	namespace {
		inline void addModifications(
				const std::vector<model::CosignatoryModification>& builderModifications,
				model::CosignatoryModification* pTransactionModifications) {
			if (!builderModifications.empty()) {
				for (const auto& modification : builderModifications) {
					*pTransactionModifications = modification;
					++pTransactionModifications;
				}
			}
		}
	}

	template<typename TransactionType>
	std::unique_ptr<TransactionType> ModifyContractBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto size = sizeof(TransactionType)
			+ m_customerModifications.size() * sizeof(model::CosignatoryModification)
			+ m_executorModifications.size() * sizeof(model::CosignatoryModification)
			+ m_verifierModifications.size() * sizeof(model::CosignatoryModification);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set transaction fields
		pTransaction->DurationDelta = m_durationDelta;
		pTransaction->Hash = m_hash;

		// 3. set sizes upfront, so that pointers are calculated correctly
		pTransaction->CustomerModificationCount = utils::checked_cast<size_t, uint8_t>(m_customerModifications.size());
		pTransaction->ExecutorModificationCount = utils::checked_cast<size_t, uint8_t>(m_executorModifications.size());
		pTransaction->VerifierModificationCount = utils::checked_cast<size_t, uint8_t>(m_verifierModifications.size());

		// 4. set modifications
		addModifications(m_customerModifications, pTransaction->CustomerModificationsPtr());
		addModifications(m_executorModifications, pTransaction->ExecutorModificationsPtr());
		addModifications(m_verifierModifications, pTransaction->VerifierModificationsPtr());

		return pTransaction;
	}

	std::unique_ptr<ModifyContractBuilder::Transaction> ModifyContractBuilder::build() const {
		return buildImpl<Transaction>();
	}

	std::unique_ptr<ModifyContractBuilder::EmbeddedTransaction> ModifyContractBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
