/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndDriveVerificationBuilder.h"

namespace catapult { namespace builders {

    EndDriveVerificationBuilder::EndDriveVerificationBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
            : TransactionBuilder(networkIdentifier, signer)
            , m_shardId(0)
            , m_keyCount(0)
            , m_judgingKeyCount(0)
    {}

    void EndDriveVerificationBuilder::setDriveKey(const Key& driveKey) {
        m_driveKey = driveKey;
    }

    void EndDriveVerificationBuilder::setVerificationTrigger(const Hash256& verificationTrigger) {
        m_verificationTrigger = verificationTrigger;
    }

    void EndDriveVerificationBuilder::setShardId(uint16_t shardId) {
        m_shardId = shardId;
    }

    void EndDriveVerificationBuilder::setKeyCount(uint8_t keyCount) {
        m_keyCount = keyCount;
    }

    void EndDriveVerificationBuilder::setJudgingKeyCount(uint8_t judgingKeyCount) {
        m_judgingKeyCount = judgingKeyCount;
    }

    void EndDriveVerificationBuilder::setPublicKeys(std::vector<Key>&& keys) {
        m_publicKeys = std::move(keys);
    }

    void EndDriveVerificationBuilder::setSignatures(std::vector<Signature>&& signatures) {
        m_signatures = std::move(signatures);
    }

    void EndDriveVerificationBuilder::setOpinions(std::vector<uint8_t>&& opinions) {
        m_opinions = std::move(opinions);
    }

    template<typename TransactionType>
    model::UniqueEntityPtr<TransactionType> EndDriveVerificationBuilder::buildImpl() const {
        // 1. allocate
        auto size = sizeof(TransactionType)
                    + m_publicKeys.size() * Key_Size
                    + m_signatures.size() * Signature_Size
                    + m_opinions.size() * sizeof(uint8_t);
        auto pTransaction = createTransaction<TransactionType>(size);

        // 2. set transaction fields
        pTransaction->DriveKey = m_driveKey;
        pTransaction->VerificationTrigger = m_verificationTrigger;
        pTransaction->ShardId = m_shardId;
        pTransaction->KeyCount = m_keyCount;
        pTransaction->JudgingKeyCount = m_judgingKeyCount;

        // 3. set transaction attachments
		
        std::copy(m_publicKeys.cbegin(), m_publicKeys.cend(), pTransaction->PublicKeysPtr());
        std::copy(m_signatures.cbegin(), m_signatures.cend(), pTransaction->SignaturesPtr());
        std::copy(m_opinions.cbegin(), m_opinions.cend(), pTransaction->OpinionsPtr());

        return pTransaction;
    }

    model::UniqueEntityPtr<EndDriveVerificationBuilder::Transaction> EndDriveVerificationBuilder::build() const {
        return buildImpl<Transaction>();
    }

    model::UniqueEntityPtr<EndDriveVerificationBuilder::EmbeddedTransaction> EndDriveVerificationBuilder::buildEmbedded() const {
        return buildImpl<EmbeddedTransaction>();
    }

}}
