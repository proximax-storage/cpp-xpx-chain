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
			, m_hash()
	{}

	void HashLockBuilder::setMosaic(UnresolvedMosaicId mosaicId, Amount amount) {
		m_mosaic = { mosaicId, amount };
	}

	void HashLockBuilder::setDuration(BlockDuration duration) {
		m_duration = duration;
	}

	void HashLockBuilder::setHash(const Hash256& hash) {
		m_hash = hash;
	}

	template<typename TransactionType>
	std::unique_ptr<TransactionType> HashLockBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType));

		// 2. set transaction fields
		pTransaction->Mosaic = m_mosaic;
		pTransaction->Duration = m_duration;
		pTransaction->Hash = m_hash;

		return pTransaction;
	}

	std::unique_ptr<HashLockBuilder::Transaction> HashLockBuilder::build() const {
		return buildImpl<Transaction>();
	}

	std::unique_ptr<HashLockBuilder::EmbeddedTransaction> HashLockBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
}}
