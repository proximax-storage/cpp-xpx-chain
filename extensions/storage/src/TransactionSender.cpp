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
    catapult::Signature TransactionSender::sendDataModificationApprovalTransaction(const crypto::KeyPair& sender,
                                                                    const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending data modification approval transaction " << transactionInfo.m_modifyTransactionHash.data();

        builders::DataModificationApprovalBuilder builder(m_networkIdentifier, sender.publicKey());
        builder.setDriveKey(transactionInfo.m_driveKey);
        builder.setDataModificationId(transactionInfo.m_modifyTransactionHash);
        builder.setFileStructureCdi(transactionInfo.m_rootHash);
        builder.setFileStructureSize(transactionInfo.m_fsTreeFileSize);
        builder.setUsedDriveSize(transactionInfo.m_driveSize);

        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
        extensions::TransactionExtensions(m_generationHash).sign(sender, *pTransaction);

        send(sender, pTransaction);

        return pTransaction->Signature;
    }

    catapult::Signature TransactionSender::sendDataModificationSingleApprovalTransaction(const crypto::KeyPair& sender,
                                                                          const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending data modification single approval transaction " << transactionInfo.m_modifyTransactionHash.data();

        auto singleOpinion = transactionInfo.m_opinions.at(0);
        std::vector<Key> keys;
        keys.reserve(singleOpinion.m_uploadLayout.size());

        // TODO count percents here?
        // in percent
        std::vector<uint8_t> opinions;
        keys.reserve(singleOpinion.m_uploadLayout.size());

        for (const auto& layout : singleOpinion.m_uploadLayout) {
            keys.emplace_back(layout.m_key);
        }

        builders::DataModificationSingleApprovalBuilder builder(m_networkIdentifier, sender.publicKey());
        builder.setDriveKey(transactionInfo.m_driveKey);
        builder.setDataModificationId(transactionInfo.m_modifyTransactionHash);
        builder.setUsedDriveSize(transactionInfo.m_driveSize);
        builder.setUploaderKeys(keys);
        builder.setUploadOpinion(opinions);

        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
        extensions::TransactionExtensions(m_generationHash).sign(sender, *pTransaction);

        send(sender, pTransaction);

        return pTransaction->Signature;
    }

    catapult::Signature TransactionSender::sendDownloadApprovalTransaction(const crypto::KeyPair& sender,
                                                            const sirius::drive::DownloadApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending download approval transaction " << transactionInfo.m_blockHash.data();

        std::set<Key> pubKeys;

        std::vector<uint8_t> opinionIndices;
        opinionIndices.reserve(transactionInfo.m_opinions.size());

        std::vector<BLSSignature> signatures;
        signatures.reserve(transactionInfo.m_opinions.size());

        // TODO set them properly
        std::vector<uint8_t> presentOpinions;
        std::vector<uint64_t> opinions;

        auto judging = 0;
        for (const auto& opinion : transactionInfo.m_opinions) {
            opinionIndices.emplace_back(judging);
            pubKeys.insert(opinion.m_replicatorKey);
//            signatures.emplace_back(opinion.m_signature);

            for (const auto& layout : opinion.m_downloadLayout) {
                pubKeys.insert(layout.m_key);
                opinions.emplace_back(layout.m_uploadedBytes);
            }

            judging++;
        }

        builders::DownloadApprovalBuilder builder(m_networkIdentifier, sender.publicKey());
        builder.setDownloadChannelId(transactionInfo.m_downloadChannelId);
        builder.setReplicatorsKeys(std::vector<Key>(pubKeys.begin(), pubKeys.end()));
        builder.setOpinionIndices(opinionIndices);
        builder.setBlsSignatures(signatures);
        builder.setPresentOpinions(presentOpinions);
        builder.setOpinions(opinions);

        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
        extensions::TransactionExtensions(m_generationHash).sign(sender, *pTransaction);

        send(sender, pTransaction);

        return pTransaction->Signature;
    }

    void TransactionSender::send(const crypto::KeyPair& sender, std::shared_ptr<model::Transaction> pTransaction) {
        auto range = model::TransactionRange::FromEntity(pTransaction);
        m_transactionRangeHandler({std::move(range), sender.publicKey()});
    }
}}
