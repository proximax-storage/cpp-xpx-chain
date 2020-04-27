/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractTypes.h"
#include "SuperContractEntityType.h"
#include "catapult/model/Transaction.h"
#include "catapult/utils/ArraySet.h"

namespace catapult { namespace config { class BlockchainConfiguration; } }

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for a suspend transaction header.
    template<typename THeader>
    struct SuspendTransactionBody : public THeader {
    private:
        using TransactionType = SuspendTransactionBody<THeader>;

    public:
        DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Suspend, 1)

    public:
        /// Key of super contract account.
        Key SuperContract;

    public:
        // Calculates the real size of a suspend transaction.
        static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
            return sizeof(TransactionType);
        }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(Suspend)

#pragma pack(pop)
}}
