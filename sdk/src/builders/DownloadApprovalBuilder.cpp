/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadApprovalBuilder.h"

namespace catapult { namespace builders {

    DownloadApprovalBuilder::DownloadApprovalBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
            : TransactionBuilder(networkIdentifier, signer)
    {}

//    void DownloadApprovalBuilder::setDriveKey(const Key& driveKey) {
//        m_driveKey = driveKey;
//    }

    void DownloadApprovalBuilder::setDownloadChannelId(const Hash256& downloadChannelId) {
        m_downloadChannelId = downloadChannelId;
    }

    void DownloadApprovalBuilder::setSequenceNumber(uint16_t sequenceNumber) {
        m_sequenceNumber = sequenceNumber;
    }

    void DownloadApprovalBuilder::setResponseToFinishDownloadTransaction(bool responseToFinishDownloadTransaction) {
        m_responseToFinishDownloadTransaction = responseToFinishDownloadTransaction;
    }

    void DownloadApprovalBuilder::setReplicatorsKeys(const std::vector<Key>& keys) {
        m_replicatorsKeys = keys;
    }

    void DownloadApprovalBuilder::setOpinionIndices(const std::vector<uint8_t>& opinionIndices) {
        m_opinionIndices = opinionIndices;
    }

    void DownloadApprovalBuilder::setBlsSignatures(const std::vector<BLSSignature>& blsSignatures) {
        m_blsSignatures = blsSignatures;
    }

    void DownloadApprovalBuilder::setPresentOpinions(const std::vector<uint8_t>& presentOpinions) {
        m_presentOpinions = presentOpinions;
    }

    void DownloadApprovalBuilder::setOpinions(const std::vector<uint64_t>& opinions) {
        m_opinions = opinions;
    }

    template<typename TransactionType>
    model::UniqueEntityPtr<TransactionType> DownloadApprovalBuilder::buildImpl() const {
        // 1. allocate
        auto pTransaction = createTransaction<TransactionType>(sizeof(TransactionType));

        // 2. set transaction fields
        pTransaction->DownloadChannelId = m_downloadChannelId;
        pTransaction->SequenceNumber = m_sequenceNumber;
        pTransaction->ResponseToFinishDownloadTransaction = m_responseToFinishDownloadTransaction;
        pTransaction->OpinionCount = m_presentOpinions.size();
        pTransaction->JudgingCount = m_opinionIndices.size();
        pTransaction->JudgedCount = m_replicatorsKeys.size();
        pTransaction->OpinionElementCount = m_opinions.size();

        // 3. set transaction attachments
        std::copy(m_replicatorsKeys.cbegin(), m_replicatorsKeys.cend(), pTransaction->PublicKeysPtr());
        std::copy(m_opinionIndices.cbegin(), m_opinionIndices.cend(), pTransaction->OpinionIndicesPtr());
        std::copy(m_blsSignatures.cbegin(), m_blsSignatures.cend(), pTransaction->BlsSignaturesPtr());
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
