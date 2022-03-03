/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadApprovalBuilder.h"

namespace catapult { namespace builders {

    DownloadApprovalBuilder::DownloadApprovalBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
            : TransactionBuilder(networkIdentifier, signer)
            , m_sequenceNumber(0)
            , m_responseToFinishDownloadTransaction(false)
            , m_judgingKeysCount(0)
            , m_overlappingKeysCount(0)
            , m_judgedKeysCount(0)
    {}

    void DownloadApprovalBuilder::setDownloadChannelId(const Hash256& downloadChannelId) {
        m_downloadChannelId = downloadChannelId;
    }

    void DownloadApprovalBuilder::setApprovalTrigger(const Hash256& approvalTrigger) {
        m_approvalTrigger = approvalTrigger;
    }

    void DownloadApprovalBuilder::setResponseToFinishDownloadTransaction(bool responseToFinishDownloadTransaction) {
        m_responseToFinishDownloadTransaction = responseToFinishDownloadTransaction;
    }

    void DownloadApprovalBuilder::setJudgingKeysCount(uint8_t judgingKeysCount) {
        m_judgingKeysCount = judgingKeysCount;
    }

    void DownloadApprovalBuilder::setOverlappingKeysCount(uint8_t overlappingKeysCount) {
        m_overlappingKeysCount = overlappingKeysCount;
    }

    void DownloadApprovalBuilder::setJudgedKeysCount(uint8_t judgedKeysCount) {
        m_judgedKeysCount = judgedKeysCount;
    }

    void DownloadApprovalBuilder::setPublicKeys(std::vector<Key>&& keys) {
        m_publicKeys = std::move(keys);
    }

    void DownloadApprovalBuilder::setSignatures(std::vector<Signature>&& signatures) {
        m_signatures = std::move(signatures);
    }

    void DownloadApprovalBuilder::setPresentOpinions(std::vector<uint8_t>&& presentOpinions) {
        m_presentOpinions = std::move(presentOpinions);
    }

    void DownloadApprovalBuilder::setOpinions(std::vector<uint64_t>&& opinions) {
        m_opinions = std::move(opinions);
    }

    template<typename TransactionType>
    model::UniqueEntityPtr<TransactionType> DownloadApprovalBuilder::buildImpl() const {
        // 1. allocate
        auto size = sizeof(TransactionType)
                    + m_publicKeys.size() * Key_Size
                    + m_signatures.size() * Signature_Size
                    + m_presentOpinions.size() * sizeof(uint8_t)
                    + m_opinions.size() * sizeof(uint64_t);
        auto pTransaction = createTransaction<TransactionType>(size);

        // 2. set transaction fields
        pTransaction->DownloadChannelId = m_downloadChannelId;
        pTransaction->ApprovalTrigger = m_approvalTrigger;
        pTransaction->ResponseToFinishDownloadTransaction = m_responseToFinishDownloadTransaction;
        pTransaction->JudgingKeysCount = m_judgingKeysCount;
        pTransaction->OverlappingKeysCount = m_overlappingKeysCount;
        pTransaction->JudgedKeysCount = m_judgedKeysCount;
        pTransaction->OpinionElementCount = utils::checked_cast<size_t, uint16_t>(m_opinions.size());

        // 3. set transaction attachments
        std::copy(m_publicKeys.cbegin(), m_publicKeys.cend(), pTransaction->PublicKeysPtr());
        std::copy(m_signatures.cbegin(), m_signatures.cend(), pTransaction->SignaturesPtr());
        std::copy(m_presentOpinions.cbegin(), m_presentOpinions.cend(), pTransaction->PresentOpinionsPtr());
        std::copy(m_opinions.cbegin(), m_opinions.cend(), pTransaction->OpinionsPtr());

        return pTransaction;
    }

    model::UniqueEntityPtr<DownloadApprovalBuilder::Transaction> DownloadApprovalBuilder::build() const {
        return buildImpl<Transaction>();
    }

    model::UniqueEntityPtr<DownloadApprovalBuilder::EmbeddedTransaction> DownloadApprovalBuilder::buildEmbedded() const {
        return buildImpl<EmbeddedTransaction>();
    }

}}
