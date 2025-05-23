/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "TransactionBuilder.h"
#include "plugins/txes/storage/src/model/DataModificationApprovalTransaction.h"
#include <vector>

namespace catapult { namespace builders {

    /// Builder for a catapult upgrade transaction.
    class DataModificationApprovalBuilder : public TransactionBuilder {
    public:
        using Transaction = model::DataModificationApprovalTransaction;
        using EmbeddedTransaction = model::EmbeddedDataModificationApprovalTransaction;

        /// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
        /// for the network specified by \a networkIdentifier.
        DataModificationApprovalBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

    public:
        void setDriveKey(const Key& driveKey);

        void setDataModificationId(const Hash256& dataModificationId);

        void setFileStructureCdi(const Hash256& fileStructureCdi);

		void setModificationStatus(uint8_t status);

        void setFileStructureSize(uint64_t fileStructureSize);

        void setMetaFilesSize(uint64_t metaFilesSize);

        void setUsedDriveSize(uint64_t usedDriveSize);

        void setJudgingKeysCount(uint8_t judgingKeysCount);

        void setOverlappingKeysCount(uint8_t overlappingKeysCount);

        void setJudgedKeysCount(uint8_t judgedKeysCount);

        void setPublicKeys(std::vector<Key>&& publicKeys);

        void setSignatures(std::vector<Signature>&& signatures);

        void setPresentOpinions(std::vector<uint8_t>&& presentOpinions);

        void setOpinions(std::vector<uint64_t>&& opinions);

    public:
        /// Builds a new data modification approval transaction.
        model::UniqueEntityPtr<Transaction> build() const;

        /// Builds a new embedded data modification approval transaction.
        model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

    private:
        template<typename TTransaction>
        model::UniqueEntityPtr<TTransaction> buildImpl() const;

    private:
        Key m_driveKey;
        Hash256 m_dataModificationId;
        Hash256 m_fileStructureCdi;
		uint8_t m_modificationStatus;
        uint64_t m_fileStructureSize;
        uint64_t m_metaFilesSize;
        uint64_t m_usedDriveSize;
        uint8_t m_judgingKeysCount;
        uint8_t m_overlappingKeysCount;
        uint8_t m_judgedKeysCount;
        std::vector<Key> m_publicKeys;
        std::vector<Signature> m_signatures;
        std::vector<uint8_t> m_presentOpinions;
        std::vector<uint64_t> m_opinions;
    };
}}
