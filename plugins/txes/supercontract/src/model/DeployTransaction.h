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

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for a deploy transaction header.
    template<typename THeader>
    struct DeployTransactionBody : public THeader {
    private:
        using TransactionType = DeployTransactionBody<THeader>;

    public:
        DEFINE_TRANSACTION_CONSTANTS(Entity_Type_Deploy, 1)

    public:
        /// Key of drive.
        Key DriveKey;

        /// Owner of super contract.
        Key Owner;

        /// A hash of according super contract file on drive.
        Hash256 FileHash;

        /// A version of super contract.
        catapult::VmVersion VmVersion;

    public:
        // Calculates the real size of a deploy transaction.
        static constexpr uint64_t CalculateRealSize(const TransactionType&) noexcept {
            return sizeof(TransactionType);
        }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(Deploy)

#pragma pack(pop)

    /// Extracts public keys of additional accounts that must approve \a transaction.
    inline utils::KeySet ExtractAdditionalRequiredCosigners(const EmbeddedDeployTransaction& transaction) {
        return { transaction.Owner };
    }
}}
