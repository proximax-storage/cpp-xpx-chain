/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <utility>

#include "StorageConfiguration.h"
#include "src/catapult/crypto/KeyPair.h"
#include "catapult/utils/NetworkTime.h"
#include "catapult/extensions/ServerHooks.h"
#include "drive/FlatDrive.h"

namespace catapult { namespace storage {
    class TransactionSender {
    public:
        TransactionSender(
                model::NetworkIdentifier networkIdentifier,
                GenerationHash generationHash,
                handlers::TransactionRangeHandler transactionRangeHandler,
                StorageConfiguration storageConfig)
                : m_networkIdentifier(networkIdentifier)
                , m_generationHash(generationHash)
                , m_transactionRangeHandler(std::move(transactionRangeHandler))
                , m_storageConfig(std::move(storageConfig)) {}

    public:
        void sendDataModificationApprovalTransaction(const crypto::KeyPair& sender, const sirius::drive::ApprovalTransactionInfo& transactionInfo);
        void sendDataModificationSingleApprovalTransaction(const crypto::KeyPair& sender, const sirius::drive::ApprovalTransactionInfo& transactionInfo);

    private:
        void signAndSend(const crypto::KeyPair& sender, std::shared_ptr<model::Transaction> pTransaction);

    private:
        GenerationHash m_generationHash;
        StorageConfiguration m_storageConfig;
        model::NetworkIdentifier m_networkIdentifier;
        handlers::TransactionRangeHandler m_transactionRangeHandler;
    };
}}
