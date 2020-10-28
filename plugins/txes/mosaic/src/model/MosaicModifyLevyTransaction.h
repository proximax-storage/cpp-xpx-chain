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
	
		/// Binary layout for a  modify levy transaction body.
		template<typename THeader>
		struct MosaicModifyLevyTransactionBody : public THeader {
		private:
			using TransactionType = MosaicModifyLevyTransactionBody<THeader>;

		public:
			DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Mosaic_Modify_Levy, 1)
			
		public:

			/// Id of the mosaic.
			UnresolvedMosaicId MosaicId;

			/// levy container
			model::MosaicLevyRaw Levy;

		public:
			/// Calculates the real size of modify levy \a transaction.
			static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
				return sizeof(TransactionType);
			}
		};

		DEFINE_EMBEDDABLE_TRANSACTION(MosaicModifyLevy)

#pragma pack(pop)
}}