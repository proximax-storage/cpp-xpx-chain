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

        void setFileStructureSize(uint64_t fileStructureSize);

        void setUsedDriveSize(uint64_t usedDriveSize);

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
        uint64_t m_fileStructureSize;
        uint64_t m_usedDriveSize;
    };
}}
