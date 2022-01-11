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

        /// Shard identifier.
        uint16_t ShardId;

		/// Total number of replicators.
		uint8_t KeyCount;

		/// Number of replicators that provided their opinions.
		uint8_t JudgingKeyCount;

		/// Replicators' public keys.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(PublicKeys, Key)

		/// Signatures of replicators' opinions.
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Signatures, Signature)

		/// Two-dimensional bit array of opinions (1 is success, 0 is failure).
		DEFINE_TRANSACTION_VARIABLE_DATA_ACCESSORS(Opinions, uint8_t)

	private:
        template<typename T>
        static auto* PublicKeysPtrT(T& transaction) {
            return transaction.KeyCount ? THeader::PayloadStart(transaction) : nullptr;
        }

        template<typename T>
        static auto* SignaturesPtrT(T& transaction) {
            auto* pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.JudgingKeyCount && pPayloadStart ? pPayloadStart + transaction.KeyCount * Key_Size : nullptr;
        }

        template<typename T>
        static auto* OpinionsPtrT(T& transaction) {
            auto* pPayloadStart = THeader::PayloadStart(transaction);
            return transaction.KeyCount && transaction.JudgingKeyCount && pPayloadStart ? pPayloadStart
				+ transaction.KeyCount * Key_Size + transaction.JudgingKeyCount * Signature_Size : nullptr;
        }

	public:
        // Calculates the real size of a storage \a transaction.
        static constexpr uint64_t CalculateRealSize(const TransactionType& transaction) noexcept {
            return sizeof(TransactionType)
                   + transaction.KeyCount * Key_Size
                   + transaction.JudgingKeyCount * Signature_Size
                   + (transaction.JudgingKeyCount * transaction.KeyCount + 7) / 8;
        }
    };

    DEFINE_EMBEDDABLE_TRANSACTION(EndDriveVerification)

#pragma pack(pop)
}}
