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

#include "MosaicPropertyBuilder.h"

namespace catapult { namespace builders {

	MosaicPropertyBuilder::MosaicPropertyBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
			, m_propertyType()
			, m_modifications()
	{}

	void MosaicPropertyBuilder::setPropertyType(model::PropertyType propertyType) {
		m_propertyType = propertyType;
	}

	void MosaicPropertyBuilder::addModification(const model::MosaicPropertyModification& modification) {
		m_modifications.push_back(modification);
	}

	model::UniqueEntityPtr<MosaicPropertyBuilder::Transaction> MosaicPropertyBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<MosaicPropertyBuilder::EmbeddedTransaction> MosaicPropertyBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> MosaicPropertyBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto size = sizeof(TransactionType);
		size += m_modifications.size() * sizeof(model::MosaicPropertyModification);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set fixed transaction fields
		pTransaction->PropertyType = m_propertyType;
		pTransaction->ModificationsCount = utils::checked_cast<size_t, uint8_t>(m_modifications.size());

		// 3. set transaction attachments
		std::copy(m_modifications.cbegin(), m_modifications.cend(), pTransaction->ModificationsPtr());

		return pTransaction;
	}
}}
