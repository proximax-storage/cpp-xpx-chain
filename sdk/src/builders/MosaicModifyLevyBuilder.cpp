/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "MosaicModifyLevyBuilder.h"
#include "plugins/txes/mosaic/src/model/MosaicIdGenerator.h"

namespace catapult { namespace builders {
		
	MosaicModifyLevyBuilder::MosaicModifyLevyBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
		, m_mosaicId(UnresolvedMosaicId())
		, m_levy(model::MosaicLevyRaw())
	{}
	
	void MosaicModifyLevyBuilder::setMosaicId(const UnresolvedMosaicId& mosaicId) {
		m_mosaicId = mosaicId;
	}
	
	void MosaicModifyLevyBuilder::setMosaicLevy(const model::MosaicLevyRaw& levy) {
		m_levy = levy;
	}
	
	model::UniqueEntityPtr<MosaicModifyLevyBuilder::Transaction> MosaicModifyLevyBuilder::build() const {
		return buildImpl<Transaction>();
	}
	
	model::UniqueEntityPtr<MosaicModifyLevyBuilder::EmbeddedTransaction> MosaicModifyLevyBuilder::buildEmbedded() const {
		return buildImpl<EmbeddedTransaction>();
	}
	
	template<typename TransactionType>
	model::UniqueEntityPtr<TransactionType> MosaicModifyLevyBuilder::buildImpl() const {
		// 1. allocate, zero (header), set model::Transaction fields
		auto size = sizeof(TransactionType);
		auto pTransaction = createTransaction<TransactionType>(size);
		
		// 2. set transaction fields
		pTransaction->MosaicId = m_mosaicId;
		pTransaction->Levy = m_levy;
		
		return pTransaction;
	}
}}
