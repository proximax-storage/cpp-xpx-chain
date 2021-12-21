/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageTypes.h"
#include "StorageEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)
    /// Binary layout for a end drive verification transaction body.
    template<typename THeader>
    struct EndDriveVerificationTransactionBody : public THeader {
    private:
        using TransactionType = EndDriveVerificationTransactionBody<THeader>;

    public:
        explicit EndDriveVerificationTransactionBody<THeader>() = default;

    public:
        DEFINE_TRANSACTION_CONSTANTS(Entity_Type_EndDriveVerification, 1)

    public:
        /// Key of the drive.
        Key DriveKey;

        /// The hash of block that initiated the Verification.
        Hash256 VerificationTrigger;

        /// Number of Provers.
        uint16_t ProversCount;

        /// Number of Verification opinions.
        uint16_t VerificationOpinionsCount;

        /// Public Keys of the Provers.
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Provers, Key)

        /// Verification Results.
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(VerificationOpinions, VerificationOpinion)

    public:
        template<typename T>
        static auto* ProversPtrT(T& transaction) {
            return transaction.ProversCount ? THeader::PayloadStart(transaction) : nullptr;
        }

        template<typename T>
        static auto* VerificationOpinionsPtrT(T& transaction) {
            auto* pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.VerificationOpinionsCount && pPayloadStart ? pPayloadStart
                    + transaction.ProversCount * sizeof(Key) : nullptr;
        }

        // Calculates the real size of a storage \a transaction.
        static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
            return sizeof(TransactionType)
                   + transaction.ProversCount * sizeof(Key)
                   + transaction.VerificationOpinionsCount * (sizeof(uint16_t) + sizeof(Signature))
                   + transaction.VerificationOpinionsCount * transaction.ProversCount * (sizeof(uint16_t) + sizeof(uint8_t)); // size of VerificationOpinion.Results
        }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(EndDriveVerification)

#pragma pack(pop)
}}
