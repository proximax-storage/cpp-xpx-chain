/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SuperContractEntityType.h"
#include "plugins/txes/operation/src/model/EndOperationTransaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for an end execute transaction header.
    template<typename THeader>
    struct EndExecuteTransactionBody : public EndOperationTransactionBody<THeader> {
    public:
        DEFINE_TRANSACTION_CONSTANTS(Entity_Type_EndExecute, 1)
    };

    DEFINE_EMBEDDABLE_TRANSACTION(EndExecute)

#pragma pack(pop)

    /// Extracts public keys of additional accounts that must approve \a transaction.
    inline utils::KeySet ExtractAdditionalRequiredCosigners(const EmbeddedEndExecuteTransaction&, const config::BlockchainConfiguration&) {
        return {};
    }
}}
