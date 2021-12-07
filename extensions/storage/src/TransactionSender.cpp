/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <boost/dynamic_bitset.hpp>
#include "TransactionSender.h"
#include "sdk/src/builders/DataModificationApprovalBuilder.h"
#include "sdk/src/builders/DataModificationSingleApprovalBuilder.h"
#include "sdk/src/builders/DownloadApprovalBuilder.h"
#include "sdk/src/extensions/TransactionExtensions.h"

namespace catapult { namespace storage {
    catapult::Signature TransactionSender::sendDataModificationApprovalTransaction(
            const crypto::KeyPair& sender,
            const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending data modification approval transaction "
                            << transactionInfo.m_modifyTransactionHash.data();

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

    catapult::Signature TransactionSender::sendDataModificationSingleApprovalTransaction(
            const crypto::KeyPair& sender,
            const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending data modification single approval transaction "
                            << transactionInfo.m_modifyTransactionHash.data();

        auto singleOpinion = transactionInfo.m_opinions.at(0);
        std::vector<Key> keys;
        keys.reserve(singleOpinion.m_uploadLayout.size());

        // TODO count percents here?
        // in percent
        std::vector<uint8_t> opinions;
        keys.reserve(singleOpinion.m_uploadLayout.size());

        for (const auto& layout: singleOpinion.m_uploadLayout) {
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

    catapult::Signature TransactionSender::sendDownloadApprovalTransaction(
            const crypto::KeyPair& sender,
            uint16_t sequenceNumber,
            const sirius::drive::DownloadApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending download approval transaction " << transactionInfo.m_blockHash.data();

        std::vector<uint8_t> opinionIndices;
        opinionIndices.reserve(transactionInfo.m_opinions.size());

        std::vector<BLSSignature> signatures;
        signatures.reserve(transactionInfo.m_opinions.size());

        std::set<Key> judgingPubKeys;
        std::set<Key> judgedPubKeys;
        auto opinionIndex = 0;
        for (const auto& opinion: transactionInfo.m_opinions) {
            opinionIndices.emplace_back(opinionIndex);
            judgingPubKeys.insert(opinion.m_replicatorKey);
//            signatures.emplace_back(opinion.m_signature);

            for (const auto& layout: opinion.m_downloadLayout)
                judgedPubKeys.insert(layout.m_key);

            opinionIndex++;
        }

        // TODO set them properly
        std::vector<std::vector<bool>> presentOpinions;
        presentOpinions.reserve(judgingPubKeys.size());

        std::vector<std::vector<uint64_t>> opinions;
        opinions.reserve(judgingPubKeys.size());

        std::set<Key> allPubKeys = judgingPubKeys;
        allPubKeys.insert(judgedPubKeys.begin(), judgedPubKeys.end());

        for (auto row = 0; row < allPubKeys.size(); ++row) {
            opinions.at(row).reserve(judgedPubKeys.size());
            std::fill(opinions.at(row).begin(), opinions.at(row).end(), 0);

            presentOpinions.at(row).reserve(judgedPubKeys.size());
            std::fill(presentOpinions.at(row).begin(), presentOpinions.at(row).end(), false);

            if (row >= judgingPubKeys.size())
                continue;

            for (const auto& layout: transactionInfo.m_opinions[row].m_downloadLayout) {
                auto column = 0;
                for (const auto& key : allPubKeys) {
                    if (key == layout.m_key)
                        break;

                    column++;
                }

                presentOpinions[row][column] = true;
                opinions[row][column] = layout.m_uploadedBytes;
            }

            row++;
        }

        std::vector<uint64_t> flatOpinions;
        for (const auto& opinion : opinions)
            flatOpinions.insert(flatOpinions.end(), opinion.begin(), opinion.end());

        boost::dynamic_bitset<uint8_t> bitset((flatOpinions.size() + 7) / 8 );
        auto index = 0;
        for (const auto& presentOpinion : presentOpinions)
            for (const auto& value : presentOpinion) {
                bitset[index] = value;
                index++;
            }

        std::vector<uint8_t> flatPresentOpinions;
        boost::to_block_range(bitset, std::back_inserter(flatPresentOpinions));

        builders::DownloadApprovalBuilder builder(m_networkIdentifier, sender.publicKey());
        builder.setDownloadChannelId(transactionInfo.m_downloadChannelId);
        builder.setSequenceNumber(sequenceNumber);
        builder.setResponseToFinishDownloadTransaction(false); // TODO set right value
        builder.setReplicatorsKeys(std::vector<Key>(allPubKeys.begin(), allPubKeys.end()));
        builder.setOpinionIndices(opinionIndices);
        builder.setBlsSignatures(signatures);
        builder.setPresentOpinions(flatPresentOpinions);
        builder.setOpinions(flatOpinions);

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
