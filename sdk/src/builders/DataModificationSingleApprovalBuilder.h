/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once

#include "TransactionBuilder.h"
#include "plugins/txes/storage/src/model/DataModificationSingleApprovalTransaction.h"
#include <vector>

namespace catapult { namespace builders {

    /// Builder for a catapult upgrade transaction.
    class DataModificationSingleApprovalBuilder : public TransactionBuilder {
    public:
        using Transaction = model::DataModificationSingleApprovalTransaction;
        using EmbeddedTransaction = model::EmbeddedDataModificationSingleApprovalTransaction;

        /// Creates a catapult upgrade builder for building a catapult upgrade transaction from \a signer
        /// for the network specified by \a networkIdentifier.
        DataModificationSingleApprovalBuilder(model::NetworkIdentifier networkIdentifier, const Key& signer);

    public:
        void setDriveKey(const Key& driveKey);

        void setDataModificationId(const Hash256& dataModificationId);

        void setPublicKeys(const std::vector<Key>& publicKeys);

        void setOpinions(const std::vector<uint64_t>& opinions);

    public:
        /// Builds a new data modification single approval transaction.
        model::UniqueEntityPtr<Transaction> build() const;

        /// Builds a new embedded data modification single approval transaction.
        model::UniqueEntityPtr<EmbeddedTransaction> buildEmbedded() const;

    private:
        template<typename TTransaction>
        model::UniqueEntityPtr<TTransaction> buildImpl() const;

    private:
        Key m_driveKey;
        Hash256 m_dataModificationId;
        std::vector<Key> m_publicKeys;
        std::vector<uint64_t> m_opinions;
    };
}}
