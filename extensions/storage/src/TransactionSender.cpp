/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionSender.h"
#include "sdk/src/builders/DataModificationApprovalBuilder.h"
#include "sdk/src/builders/DataModificationSingleApprovalBuilder.h"
#include "sdk/src/builders/DownloadApprovalBuilder.h"
#include "catapult/extensions/TransactionExtensions.h"

namespace catapult { namespace storage {
    void TransactionSender::sendDataModificationApprovalTransaction(const crypto::KeyPair& sender,
                                                                    const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
//        CATAPULT_LOG(debug) << "sending data modification approval transaction " << transactionInfo.m_modifyTransactionHash;

        builders::DataModificationApprovalBuilder builder(m_networkIdentifier, sender.publicKey());
        builder.setDriveKey(transactionInfo.m_driveKey);
        builder.setDataModificationId(transactionInfo.m_modifyTransactionHash);
        builder.setFileStructureCdi(transactionInfo.m_rootHash);
        builder.setFileStructureSize(transactionInfo.m_fsTreeFileSize);
        builder.setUsedDriveSize(transactionInfo.m_driveSize);

        auto pTransaction = utils::UniqueToShared(builder.build());
        signAndSend(sender, pTransaction);
    }

    void TransactionSender::sendDataModificationSingleApprovalTransaction(const crypto::KeyPair& sender,
                                                                          const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
//        CATAPULT_LOG(debug) << "sending data modification single approval transaction " << transactionInfo.m_modifyTransactionHash;

        builders::DataModificationSingleApprovalBuilder builder(m_networkIdentifier, sender.publicKey());
        builder.setDriveKey(transactionInfo.m_driveKey);
        builder.setDataModificationId(transactionInfo.m_modifyTransactionHash);
        builder.setUsedDriveSize(transactionInfo.m_driveSize);
//        builder.setUploadOpinion();

        auto keysArray = transactionInfo.m_opinions.at(0).m_uploadReplicatorKeys;
        std::vector<Key> keys;
        keys.reserve(keysArray.size() / Key_Size);

        auto keyNumber = 0;
        for (auto it = keysArray.begin(); it != keysArray.end(); it + Key_Size) {
            std::copy(it, it + Key_Size, keys.at(keyNumber).begin());
            keyNumber++;
        }
        builder.setUploaderKeys(keys);

        auto pTransaction = utils::UniqueToShared(builder.build());
        signAndSend(sender, pTransaction);
    }

    void TransactionSender::sendDownloadApprovalTransaction(const crypto::KeyPair& sender,
                                                            const sirius::drive::DownloadApprovalTransactionInfo& transactionInfo) {
//        CATAPULT_LOG(debug) << "sending download approval transaction " << transactionInfo.m_blockHash;

        builders::DownloadApprovalBuilder builder(m_networkIdentifier, sender.publicKey());
        builder.setDownloadChannelId(transactionInfo.m_downloadChannelId);

        // TODO set other fields
        std::vector<Key> pubKeys;
        pubKeys.reserve(transactionInfo.m_opinions.size());

        std::vector<uint8_t> opinionIndices;
        opinionIndices.reserve(transactionInfo.m_opinions.size());

        std::vector<BLSSignature> signatures;
        signatures.reserve(transactionInfo.m_opinions.size());

        std::vector<uint64_t> opinions;
        opinions.reserve(transactionInfo.m_opinions.size());

        auto index = 0;
        for (const auto& opinion : transactionInfo.m_opinions) {
            opinionIndices.emplace_back(index);

            pubKeys.emplace_back(opinion.m_replicatorKey);
//            signatures.emplace_back(opinion.m_signature);
//            opinions.emplace_back(opinion.m_downloadedBytes);

            index++;
        }

        auto pTransaction = utils::UniqueToShared(builder.build());
        signAndSend(sender, pTransaction);
    }

    void TransactionSender::signAndSend(const crypto::KeyPair& sender, std::shared_ptr<model::Transaction> pTransaction) {
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
        extensions::TransactionExtensions(m_generationHash).sign(sender, *pTransaction);

        auto range = model::TransactionRange::FromEntity(pTransaction);
        m_transactionRangeHandler({std::move(range), sender.publicKey()});
    }
}}
