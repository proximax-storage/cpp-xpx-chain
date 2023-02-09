/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/
#pragma once
#include "catapult/model/Transaction.h"
#include "MosaicEntityType.h"
#include "src/model/MosaicLevy.h"

namespace catapult { namespace model {

#pragma pack(push, 1)
	
	/// Binary layout for a remove levy transaction body.
	template<typename THeader>
	struct MosaicRemoveLevyTransactionBody : public THeader {
	private:
		using TransactionType = MosaicRemoveLevyTransactionBody<THeader>;
	
	public:
		DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Mosaic_Remove_Levy, 1)
		
	public:
		
		/// Id of the mosaic.
		UnresolvedMosaicId MosaicId;
		
	public:
		/// Calculates the real size of remove levy \a transaction.
		static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
			return sizeof(TransactionType);
		}
	};
	
	DEFINE_EMBEDDABLE_TRANSACTION(MosaicRemoveLevy)
	
#pragma pack(pop)

}}