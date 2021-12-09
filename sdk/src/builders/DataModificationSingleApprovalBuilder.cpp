/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DataModificationSingleApprovalBuilder.h"

namespace catapult { namespace builders {

    DataModificationSingleApprovalBuilder::DataModificationSingleApprovalBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
            : TransactionBuilder(networkIdentifier, signer)
    {}

    void DataModificationSingleApprovalBuilder::setDriveKey(const Key& driveKey) {
        m_driveKey = driveKey;
    }

    void DataModificationSingleApprovalBuilder::setDataModificationId(const Hash256& dataModificationId) {
        m_dataModificationId = dataModificationId;
    }

    void DataModificationSingleApprovalBuilder::setUploaderKeys(const std::vector<Key>& uploaderKeys) {
        m_uploaderKeys = uploaderKeys;
    }

    void DataModificationSingleApprovalBuilder::setUploadOpinion(const std::vector<uint8_t>& uploadOpinions) {
        m_uploadOpinions = uploadOpinions;
    }

    void DataModificationSingleApprovalBuilder::setUsedDriveSize(uint64_t usedDriveSize) {
        m_usedDriveSize = usedDriveSize;
    }

    template<typename TransactionType>
    model::UniqueEntityPtr<TransactionType> DataModificationSingleApprovalBuilder::buildImpl() const {
        // 1. allocate
        auto size = sizeof(TransactionType)
                + m_uploaderKeys.size() * sizeof(Key)
                + m_uploadOpinions.size() * sizeof(uint8_t);
        auto pTransaction = createTransaction<TransactionType>(size);

        // 2. set transaction fields
        pTransaction->DriveKey = m_driveKey;
        pTransaction->DataModificationId = m_dataModificationId;
        pTransaction->UploadOpinionPairCount = m_uploadOpinions.size();
        pTransaction->UsedDriveSize = m_usedDriveSize;

        // 3. set transaction attachments
        std::copy(m_uploaderKeys.cbegin(), m_uploaderKeys.cend(), pTransaction->UploaderKeysPtr());
        std::copy(m_uploadOpinions.cbegin(), m_uploadOpinions.cend(), pTransaction->UploadOpinionPtr());

        return pTransaction;
    }

    model::UniqueEntityPtr<DataModificationSingleApprovalBuilder::Transaction> DataModificationSingleApprovalBuilder::build() const {
        return buildImpl<Transaction>();
    }

    model::UniqueEntityPtr<DataModificationSingleApprovalBuilder::EmbeddedTransaction> DataModificationSingleApprovalBuilder::buildEmbedded() const {
        return buildImpl<EmbeddedTransaction>();
    }
}}
