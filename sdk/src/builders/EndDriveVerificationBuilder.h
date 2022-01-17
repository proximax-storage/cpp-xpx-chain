/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "TransactionBuilder.h"
#include "plugins/txes/storage/src/model/EndDriveVerificationTransaction.h"
#include <vector>

namespace catapult { namespace builders {

    /// Builder for a catapult upgrade transaction.
    class EndDriveVerificationBuilder : public TransactionBuilder {
    public:
        using Transaction = model::EndDriveVerificationTransaction;
        using EmbeddedTransaction = model::EmbeddedEndDriveVerificationTransaction;

        /// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
        /// for the network specified by \a networkIdentifier.
        EndDriveVerificationBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

    public:
        void setDriveKey(const Key& driveKey);
        void setVerificationTrigger(const Hash256& verificationTrigger);
        void setShardId(uint16_t shardId);
        void setKeyCount(uint8_t keyCount);
        void setJudgingKeyCount(uint8_t judgingKeyCount);

        void setPublicKeys(std::vector<Key>&& keys);
        void setSignatures(std::vector<Signature>&& signatures);
        void setOpinions(std::vector<uint8_t>&& opinions);

    public:
        /// Builds a new download approval approval transaction.
        model::UniqueEntityPtr<Transaction> build() const;

        /// Builds a new embedded download approval transaction.
        model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

    private:
        template<typename TTransaction>
        model::UniqueEntityPtr<TTransaction> buildImpl() const;

    private:
        Key m_driveKey;
        Hash256 m_verificationTrigger;
        uint16_t m_shardId;
        uint8_t m_keyCount;
        uint8_t m_judgingKeyCount;
        std::vector<Key> m_publicKeys;
        std::vector<Signature> m_signatures;
        std::vector<uint8_t> m_opinions;
    };
}}
