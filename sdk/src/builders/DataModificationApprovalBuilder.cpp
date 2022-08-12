/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DataModificationApprovalBuilder.h"

namespace catapult { namespace builders {

    DataModificationApprovalBuilder::DataModificationApprovalBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer)
		: TransactionBuilder(networkIdentifier, signer)
		, m_fileStructureSize(0)
		, m_usedDriveSize(0)
		, m_metaFilesSize(0)
		, m_judgingKeysCount(0)
		, m_overlappingKeysCount(0)
		, m_judgedKeysCount(0)
	{}

    void DataModificationApprovalBuilder::setDriveKey(const Key& driveKey) {
        m_driveKey = driveKey;
    }

    void DataModificationApprovalBuilder::setDataModificationId(const Hash256& dataModificationId) {
        m_dataModificationId = dataModificationId;
    }

    void DataModificationApprovalBuilder::setFileStructureCdi(const Hash256& fileStructureCdi) {
        m_fileStructureCdi = fileStructureCdi;
    }

    void DataModificationApprovalBuilder::setFileStructureSize(uint64_t fileStructureSize) {
        m_fileStructureSize = fileStructureSize;
    }

    void DataModificationApprovalBuilder::setMetaFilesSize(uint64_t metaFilesSize) {
        m_metaFilesSize = metaFilesSize;
    }

    void DataModificationApprovalBuilder::setUsedDriveSize(uint64_t usedDriveSize) {
        m_usedDriveSize = usedDriveSize;
    }

    void DataModificationApprovalBuilder::setJudgingKeysCount(uint8_t judgingKeysCount) {
        m_judgingKeysCount = judgingKeysCount;
    }

    void DataModificationApprovalBuilder::setOverlappingKeysCount(uint8_t overlappingKeysCount) {
        m_overlappingKeysCount = overlappingKeysCount;
    }

    void DataModificationApprovalBuilder::setJudgedKeysCount(uint8_t judgedKeysCount) {
        m_judgedKeysCount = judgedKeysCount;
    }

    void DataModificationApprovalBuilder::setPublicKeys(std::vector<Key>&& publicKeys) {
        m_publicKeys = std::move(publicKeys);
    }

    void DataModificationApprovalBuilder::setSignatures(std::vector<Signature>&& signatures) {
        m_signatures = std::move(signatures);
    }

    void DataModificationApprovalBuilder::setPresentOpinions(std::vector<uint8_t>&& presentOpinions) {
        m_presentOpinions = std::move(presentOpinions);
    }

    void DataModificationApprovalBuilder::setOpinions(std::vector<uint64_t>&& opinions) {
        m_opinions = std::move(opinions);
    }

    template<typename TransactionType>
    model::UniqueEntityPtr<TransactionType> DataModificationApprovalBuilder::buildImpl() const {
        // 1. allocate
        auto size = sizeof(TransactionType)
                    + m_publicKeys.size() * Key_Size
                    + m_signatures.size() * Signature_Size
                    + m_presentOpinions.size() * sizeof(uint8_t)
                    + m_opinions.size() * sizeof(uint64_t);
        auto pTransaction = createTransaction<TransactionType>(size);

        // 2. set transaction fields
        pTransaction->DriveKey = m_driveKey;
        pTransaction->DataModificationId = m_dataModificationId;
        pTransaction->FileStructureCdi = m_fileStructureCdi;
        pTransaction->FileStructureSizeBytes = m_fileStructureSize;
        pTransaction->MetaFilesSizeBytes = m_metaFilesSize;
        pTransaction->UsedDriveSizeBytes = m_usedDriveSize;
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

    model::UniqueEntityPtr<DataModificationApprovalBuilder::Transaction>
    DataModificationApprovalBuilder::build() const {
        return buildImpl<Transaction>();
    }

    model::UniqueEntityPtr<DataModificationApprovalBuilder::EmbeddedTransaction>
    DataModificationApprovalBuilder::buildEmbedded() const {
        return buildImpl<EmbeddedTransaction>();
    }
}}
