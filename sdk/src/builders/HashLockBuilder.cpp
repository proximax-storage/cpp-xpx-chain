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

#include "HashLockBuilder.h"

namespace catapult { namespace builders {

	HashLockBuilder::HashLockBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
			, m_mosaic()
			, m_duration()
			, m_hash()
	{}

	void HashLockBuilder::setMosaic(const model::UnresolvedMosaic& mosaic) {
		m_mosaic = mosaic;
	}

	void HashLockBuilder::setDuration(BlockDuration duration) {
		m_duration = duration;
	}

	void HashLockBuilder::setHash(const Hash256& hash) {
		m_hash = hash;
	}

	model::UniqueEntityPtr<HashLockBuilder::Transaction> HashLockBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<HashLockBuilder::EmbeddedTransaction> HashLockBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> HashLockBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto size = sizeof(TransactionType);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set fixed transaction fields
		pTransaction->Mosaic = m_mosaic;
		pTransaction->Duration = m_duration;
		pTransaction->Hash = m_hash;

		return pTransaction;
	}
}}
