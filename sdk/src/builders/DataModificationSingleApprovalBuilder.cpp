/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DataModificationSingleApprovalBuilder.h"

namespace catapult { namespace builders {

    DataModificationSingleApprovalBuilder::DataModificationSingleApprovalBuilder(
            model::NetworkIdentifier networkIdentifier,
            const Key& signer)
            : TransactionBuilder(networkIdentifier, signer) {}

    void DataModificationSingleApprovalBuilder::setDriveKey(const Key& driveKey) {
        m_driveKey = driveKey;
    }

    void DataModificationSingleApprovalBuilder::setDataModificationId(const Hash256& dataModificationId) {
        m_dataModificationId = dataModificationId;
    }

    void DataModificationSingleApprovalBuilder::setPublicKeys(const std::vector<Key>& publicKeys) {
        m_publicKeys = publicKeys;
    }

    void DataModificationSingleApprovalBuilder::setOpinions(const std::vector<uint64_t>& opinions) {
        m_opinions = opinions;
    }

    template<typename TransactionType>
    model::UniqueEntityPtr<TransactionType> DataModificationSingleApprovalBuilder::buildImpl() const {
        // 1. allocate
        auto size = sizeof(TransactionType)
                    + m_publicKeys.size() * sizeof(Key)
                    + m_opinions.size() * sizeof(uint64_t);
        auto pTransaction = createTransaction<TransactionType>(size);

        // 2. set transaction fields
        pTransaction->DriveKey = m_driveKey;
        pTransaction->DataModificationId = m_dataModificationId;
        pTransaction->PublicKeysCount = m_publicKeys.size();

        // 3. set transaction attachments
        std::copy(m_publicKeys.cbegin(), m_publicKeys.cend(), pTransaction->PublicKeysPtr());
        std::copy(m_opinions.cbegin(), m_opinions.cend(), pTransaction->OpinionsPtr());

        return pTransaction;
    }

    model::UniqueEntityPtr<DataModificationSingleApprovalBuilder::Transaction>
    DataModificationSingleApprovalBuilder::build() const {
        return buildImpl<Transaction>();
    }

    model::UniqueEntityPtr<DataModificationSingleApprovalBuilder::EmbeddedTransaction>
    DataModificationSingleApprovalBuilder::buildEmbedded() const {
        return buildImpl<EmbeddedTransaction>();
    }
}}
