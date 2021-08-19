/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "StorageEntityType.h"
#include "catapult/model/Transaction.h"

namespace catapult { namespace model {

#pragma pack(push, 1)

    /// Binary layout for a finish drive verification transaction body.
    template<typename THeader>
    struct FinishDriveVerificationTransactionBody : public THeader {
    private:
        using TransactionType = FinishDriveVerificationTransactionBody<THeader>;

    public:
        explicit FinishDriveVerificationTransactionBody<THeader>() = default;

    public:
        DEFINE_TRANSACTION_CONSTANTS(Entity_Type_FinishDriveVerification, 1)

    public:
        /// Key of the drive.
        Key DriveKey;

        /// The hash of block that initiated the Verification.
        Hash256 VerificationTrigger;

        /// Number of key-opinion pairs in the payload.
        uint16_t VerificationOpinionPairCount;

        /// Public Keys of the Provers.
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Provers, Key)

        /// Opinion about verification status for each Prover. Success or Failure.
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(VerificationOpinion, bool)

    public:
        template<typename T>
        static auto* ProversPtrT(T& transaction) {
            auto* pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.VerificationOpinionPairCount ? pPayloadStart : nullptr;
        }

        template<typename T>
        static auto *VerificationOpinionPtrT(T &transaction) {
            auto *pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.VerificationOpinionPairCount ?
                   pPayloadStart + transaction.VerificationOpinionPairCount * sizeof(Key) : nullptr;
        }

        // Calculates the real size of a storage \a transaction.
        static constexpr uint64_t CalculateRealSize(const TransactionType &transaction) noexcept {
            return sizeof(TransactionType) + transaction.VerificationOpinionPairCount * (sizeof(Key) + sizeof(bool));
        }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(FinishDriveVerification)

#pragma pack(pop)
}}
