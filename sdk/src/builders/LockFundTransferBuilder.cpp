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

#include "LockFundTransferBuilder.h"

namespace catapult { namespace builders {

	LockFundTransferBuilder::LockFundTransferBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
			: TransactionBuilder(networkIdentifier, signer)
			, m_action()
			, m_duration()
			, m_mosaics()
	{}

	void LockFundTransferBuilder::setAction(model::LockFundAction action)
	{
		m_action = action;
	}

	void LockFundTransferBuilder::setDuration(BlockDuration duration)
	{
		m_duration = duration;
	}

	void LockFundTransferBuilder::addMosaic(const model::UnresolvedMosaic& mosaic) {
		InsertSorted(m_mosaics, mosaic, [](const auto& lhs, const auto& rhs) {
			return lhs.MosaicId < rhs.MosaicId;
		});
	}

	model::UniqueEntityPtr<LockFundTransferBuilder::Transaction> LockFundTransferBuilder::build() const {
		return buildImpl<Transaction>();
	}

	model::UniqueEntityPtr<LockFundTransferBuilder::EmbeddedTransaction> LockFundTransferBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}

	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> LockFundTransferBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto size = sizeof(TransactionType);
		size += m_mosaics.size() * sizeof(model::UnresolvedMosaic);
		auto pTransaction = createTransaction<TransactionType>(size);

		// 2. set fixed transaction fields
		pTransaction->Duration = m_duration;
		pTransaction->Action = m_action;
		pTransaction->MosaicsCount = utils::checked_cast<size_t, uint8_t>(m_mosaics.size());

		// 3. set transaction attachments
		std::copy(m_mosaics.cbegin(), m_mosaics.cend(), pTransaction->MosaicsPtr());

		return pTransaction;
	}
}}
