/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
	model::UniqueEntityPtr<TransactionType> ModifyContractBuilder::buildImpl() const {
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

	model::UniqueEntityPtr<ModifyContractBuilder::Transaction> ModifyContractBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<ModifyContractBuilder::EmbeddedTransaction> ModifyContractBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
