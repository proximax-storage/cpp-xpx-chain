/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractEntityType.h"
#include "catapult/model/Mosaic.h"
#include "catapult/model/Transaction.h"

namespace catapult::model {

#pragma pack(push, 1)

    /// Binary layout for a deploy transaction body.
	template<typename THeader>
	struct AutomaticExecutionsPaymentTransactionBody : public THeader {
        private:
		using TransactionType = AutomaticExecutionsPaymentTransactionBody<THeader>;

        public:
		    DEFINE_TRANSACTION_CONSTANTS(Entity_Type_AutomaticExecutionsPaymentTransaction, 1)
        
		    /// Contract public key
		    Key ContractKey;

            /// The number of prepaid automatic executions. Can be increased via special transaction.
            uint32_t AutomaticExecutionsNumber;

        public:
            // Calculates the real size of a deploy \a transaction.
            static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
			    return sizeof(TransactionType);
		    }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(AutomaticExecutionsPayment)

#pragma pack(pop)
}