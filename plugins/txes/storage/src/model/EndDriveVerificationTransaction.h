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

        /// Public Keys of the Provers.
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Provers, Key)

        /// Number of Verifiers` opinions.
        uint16_t VerifiersOpinionsCount;

        /// Aggregated BLS signatures of opinions.
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(BlsSignatures, BLSSignature)

        /// Opinion about verification status for each Prover. Success or Failure.
        DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(VerifiersOpinions, uint8_t)

    public:
        template<typename T>
        static auto *ProversPtrT(T &transaction) {
            return transaction.VerifiersOpinionsCount ? THeader::PayloadStart(transaction) : nullptr;
        }

        template<typename T>
        static auto *BlsSignaturesPtrT(T &transaction) {
            auto *pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.VerifiersOpinionsCount && pPayloadStart ? pPayloadStart
                    + transaction.VerifiersOpinionsCount * sizeof(Key) : nullptr;
        }

        template<typename T>
        static auto *VerifiersOpinionsPtrT(T &transaction) {
            auto *pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.VerifiersOpinionsCount && pPayloadStart ? pPayloadStart
                    + transaction.VerifiersOpinionsCount * sizeof(Key)
                    + transaction.VerifiersOpinionsCount * sizeof(BLSSignature) : nullptr;
        }

        // Calculates the real size of a storage \a transaction.
        static constexpr uint64_t CalculateRealSize(const TransactionType &transaction) noexcept {
            return sizeof(TransactionType)
                   + transaction.VerifiersOpinionsCount * sizeof(Key)
                   + transaction.VerifiersOpinionsCount * sizeof(BLSSignature)
                   + transaction.VerifiersOpinionsCount * transaction.ProversCount * sizeof(uint8_t);
        }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(EndDriveVerification)

#pragma pack(pop)
}}
