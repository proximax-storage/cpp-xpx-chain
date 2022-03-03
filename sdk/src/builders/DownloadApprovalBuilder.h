/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "TransactionBuilder.h"
#include "plugins/txes/storage/src/model/DownloadApprovalTransaction.h"
#include <vector>

namespace catapult { namespace builders {

    /// Builder for a catapult upgrade transaction.
    class DownloadApprovalBuilder : public TransactionBuilder {
    public:
        using Transaction = model::DownloadApprovalTransaction;
        using EmbeddedTransaction = model::EmbeddedDownloadApprovalTransaction;

        /// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
        /// for the network specified by \a networkIdentifier.
        DownloadApprovalBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

    public:
        void setDownloadChannelId(const Hash256& downloadChannelId);
        void setApprovalTrigger(const Hash256& approvalTrigger);
        void setResponseToFinishDownloadTransaction(bool responseToFinishDownloadTransaction);
        void setJudgingKeysCount(uint8_t judgingKeysCount);
        void setOverlappingKeysCount(uint8_t overlappingKeysCount);
        void setJudgedKeysCount(uint8_t judgedKeysCount);

        void setPublicKeys(std::vector<Key>&& keys);
        void setSignatures(std::vector<Signature>&& signatures);
        void setPresentOpinions(std::vector<uint8_t>&& presentOpinions);
        void setOpinions(std::vector<uint64_t>&& opinions);

    public:
        /// Builds a new download approval approval transaction.
        model::UniqueEntityPtr<Transaction> build() const;

        /// Builds a new embedded download approval transaction.
        model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

    private:
        template<typename TTransaction>
        model::UniqueEntityPtr<TTransaction> buildImpl() const;

    private:
        Hash256 m_downloadChannelId;
        Hash256 m_approvalTrigger;
        uint16_t m_sequenceNumber;
        bool m_responseToFinishDownloadTransaction;
        uint8_t m_judgingKeysCount;
        uint8_t m_overlappingKeysCount;
        uint8_t m_judgedKeysCount;
        std::vector<Key> m_publicKeys;
        std::vector<Signature> m_signatures;
        std::vector<uint8_t> m_presentOpinions;
        std::vector<uint64_t> m_opinions;
    };
}}
