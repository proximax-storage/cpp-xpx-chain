/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractTypes.h"
#include "SuperContractEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for a deactivate transaction header.
    template<typename THeader>
    struct DeactivateTransactionBody : public THeader {
    private:
        using TransactionType = DeactivateTransactionBody<THeader>;

    public:
        DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Deactivate, 1)

    public:
        /// Super contract key.
        Key SuperContract;

    public:
        // Calculates the real size of a deactivate transaction.
        static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
            return sizeof(TransactionType);
        }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(Deactivate)

#pragma pack(pop)
}}
