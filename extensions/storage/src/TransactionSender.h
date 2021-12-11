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
#include "catapult/state/StorageState.h"
#include "drive/FlatDrive.h"

namespace catapult { namespace storage {
    class TransactionSender {
    public:
        TransactionSender(
                const config::ImmutableConfiguration& immutableConfig,
                StorageConfiguration storageConfig,
                handlers::TransactionRangeHandler transactionRangeHandler)
                : m_networkIdentifier(immutableConfig.NetworkIdentifier)
                , m_generationHash(immutableConfig.GenerationHash)
                , m_storageConfig(std::move(storageConfig))
                , m_transactionRangeHandler(std::move(transactionRangeHandler)) {}

    public:
        catapult::Signature sendDataModificationApprovalTransaction(const crypto::KeyPair& sender, const sirius::drive::ApprovalTransactionInfo& transactionInfo);

        catapult::Signature sendDataModificationSingleApprovalTransaction(const crypto::KeyPair& sender, const sirius::drive::ApprovalTransactionInfo& transactionInfo);

        catapult::Signature sendDownloadApprovalTransaction(const crypto::KeyPair& sender, uint16_t sequenceNumber, const sirius::drive::DownloadApprovalTransactionInfo& transactionInfo);

    private:
        void send(const crypto::KeyPair& sender, std::shared_ptr<model::Transaction> pTransaction);

    private:
        model::NetworkIdentifier m_networkIdentifier;
        GenerationHash m_generationHash;
        StorageConfiguration m_storageConfig;
        handlers::TransactionRangeHandler m_transactionRangeHandler;
    };
}}
