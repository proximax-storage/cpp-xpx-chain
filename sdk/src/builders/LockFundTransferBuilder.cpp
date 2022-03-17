/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
