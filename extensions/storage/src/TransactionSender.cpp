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
#include "catapult/model/EntityHasher.h"

namespace catapult { namespace storage {

    Hash256 TransactionSender::sendDataModificationApprovalTransaction(const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending data modification approval transaction " << transactionInfo.m_modifyTransactionHash.data();

        // TODO looks really overcomplicated

        std::vector<Key> judgedKeys;
        std::vector<Key> judgingKeys;
        judgingKeys.reserve(transactionInfo.m_opinions.size());

        std::vector<Signature> signatures;
        signatures.reserve(transactionInfo.m_opinions.size());

        // collect judging and judged keys
        for (const auto& opinion : transactionInfo.m_opinions) {
            judgingKeys.emplace_back(opinion.m_replicatorKey);
            signatures.emplace_back(reinterpret_cast<const Signature&>(opinion.m_signature));

            for (const auto& layout: opinion.m_uploadLayout)
                judgedKeys.emplace_back(layout.m_key);
        }

        // separate overlapping keys
        auto overlappingKeysCount = 0;
        for (auto it = judgingKeys.begin(); it < judgingKeys.end(); ++it) {
            auto judgedIter = std::find(judgedKeys.begin(), judgedKeys.end(), *it);
            if (judgedIter == judgedKeys.end())
                continue;

            ++overlappingKeysCount;
            std::iter_swap(it, judgingKeys.end() - overlappingKeysCount);
            judgedKeys.erase(judgedIter);
        }

        const auto judgingKeysCount = judgingKeys.size() - overlappingKeysCount;
        const auto judgedKeysCount = judgedKeys.size();

        // merge all keys
        std::vector<Key> publicKeys = std::move(judgingKeys);
        publicKeys.insert(publicKeys.end(), judgedKeys.begin(), judgedKeys.end());

        auto totalJudgingKeysCount = judgingKeysCount+overlappingKeysCount;
        auto totalJudgedKeysCount = overlappingKeysCount+judgedKeysCount;

        // prepare present opinions vector
        boost::dynamic_bitset<std::uint8_t> presentOpinionsBitset;
        presentOpinionsBitset.reserve(totalJudgingKeysCount*totalJudgedKeysCount);

        // prepare opinions vector
        std::vector<std::pair<Key, std::vector<uint64_t>>> opinions;
        opinions.reserve(totalJudgingKeysCount);

        // fill opinions by Public Keys with zero opinions
        for (auto i = 0; i < totalJudgingKeysCount; i++)
            opinions[0].first = publicKeys[i];

        // parse opinions
        for (const auto& opinion: transactionInfo.m_opinions) {
            auto judgingKeyIter = std::find(publicKeys.begin(), publicKeys.end()-judgedKeysCount, opinion.m_replicatorKey);
            const auto judgingKeyIndex = judgingKeyIter - publicKeys.begin();

            for (const auto& layout: opinion.m_uploadLayout) {
                auto judgedKeyIter = std::find(publicKeys.begin()+judgingKeysCount, publicKeys.end(), opinion.m_replicatorKey);
                const auto judgedKeyIndex = publicKeys.end() - judgedKeyIter;

                auto presentOpinionIndex = judgingKeyIndex * totalJudgedKeysCount + judgedKeyIndex;
                presentOpinionsBitset[presentOpinionIndex] = true;

//                auto opinionIter = std::find(opinions.begin(), opinions.end(), Key(opinion.m_replicatorKey));
//                opinionIter->second.emplace_back(layout.m_uploadedBytes);
            }
        }

        // convert presentOpinionsBitset to uint8_t vector
        std::vector<uint8_t> presentOpinions;
        boost::to_block_range(presentOpinionsBitset, std::back_inserter(presentOpinions));

        // convert opinions paired vector to uint64_t vector
        std::vector<uint64_t> flatOpinions;
        for (const auto& pairedOpinions : opinions)
            for (const auto& opinion : pairedOpinions.second)
                flatOpinions.emplace_back(opinion);

        builders::DataModificationApprovalBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
        builder.setDriveKey(transactionInfo.m_driveKey);
        builder.setDataModificationId(transactionInfo.m_modifyTransactionHash);
        builder.setFileStructureCdi(transactionInfo.m_rootHash);
        builder.setFileStructureSize(transactionInfo.m_fsTreeFileSize);
        builder.setMetaFilesSize(transactionInfo.m_metaFilesSize);
        builder.setUsedDriveSize(transactionInfo.m_driveSize);
        builder.setJudgingKeysCount(judgingKeysCount);
        builder.setOverlappingKeysCount(overlappingKeysCount);
        builder.setJudgedKeysCount(judgedKeysCount);
        builder.setPublicKeys(publicKeys);
        builder.setSignatures(signatures);
        builder.setPresentOpinions(presentOpinions);
        builder.setOpinions(flatOpinions);

        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
		auto transactionHash = model::CalculateHash(*pTransaction, m_generationHash);
        send(pTransaction);

        return transactionHash;
    }

    Hash256 TransactionSender::sendDataModificationSingleApprovalTransaction(const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending data modification single approval transaction " << transactionInfo.m_modifyTransactionHash.data();

        auto singleOpinion = transactionInfo.m_opinions.at(0);

        std::vector<Key> keys;
        keys.reserve(singleOpinion.m_uploadLayout.size());

        std::vector<uint64_t> opinions;
        opinions.reserve(singleOpinion.m_uploadLayout.size());

        for (const auto& layout: singleOpinion.m_uploadLayout) {
            keys.emplace_back(layout.m_key);
            opinions.emplace_back(layout.m_uploadedBytes);
        }

        builders::DataModificationSingleApprovalBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
        builder.setDriveKey(transactionInfo.m_driveKey);
        builder.setDataModificationId(transactionInfo.m_modifyTransactionHash);
        builder.setPublicKeys(keys);
        builder.setOpinions(opinions);

        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
		auto transactionHash = model::CalculateHash(*pTransaction, m_generationHash);
        send(pTransaction);

        return transactionHash;
    }

    Hash256 TransactionSender::sendDownloadApprovalTransaction(
            uint16_t sequenceNumber,
            const sirius::drive::DownloadApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending download approval transaction " << transactionInfo.m_blockHash.data();

        // TODO copy paste

        std::vector<Key> judgedKeys;
        std::vector<Key> judgingKeys;
        judgingKeys.reserve(transactionInfo.m_opinions.size());

        std::vector<Signature> signatures;
        signatures.reserve(transactionInfo.m_opinions.size());

        // collect judging and judged keys
        for (const auto& opinion : transactionInfo.m_opinions) {
            judgingKeys.emplace_back(opinion.m_replicatorKey);
            signatures.emplace_back(reinterpret_cast<const Signature&>(opinion.m_signature));

            for (const auto& layout: opinion.m_downloadLayout)
                judgedKeys.emplace_back(layout.m_key);
        }

        // separate overlapping keys
        auto overlappingKeysCount = 0;
        for (auto it = judgingKeys.begin(); it < judgingKeys.end(); ++it) {
            auto judgedIter = std::find(judgedKeys.begin(), judgedKeys.end(), *it);
            if (judgedIter == judgedKeys.end())
                continue;

            ++overlappingKeysCount;
            std::iter_swap(it, judgingKeys.end() - overlappingKeysCount);
            judgedKeys.erase(judgedIter);
        }

        const auto judgingKeysCount = judgingKeys.size() - overlappingKeysCount;
        const auto judgedKeysCount = judgedKeys.size();

        // merge all keys
        std::vector<Key> publicKeys = std::move(judgingKeys);
        publicKeys.insert(publicKeys.end(), judgedKeys.begin(), judgedKeys.end());

        auto totalJudgingKeysCount = judgingKeysCount+overlappingKeysCount;
        auto totalJudgedKeysCount = overlappingKeysCount+judgedKeysCount;

        // prepare present opinions vector
        boost::dynamic_bitset<std::uint8_t> presentOpinionsBitset;
        presentOpinionsBitset.reserve(totalJudgingKeysCount*totalJudgedKeysCount);

        // prepare opinions vector
        std::vector<std::pair<Key, std::vector<uint64_t>>> opinions;
        opinions.reserve(totalJudgingKeysCount);

        // fill opinions by Public Keys with zero opinions
        for (auto i = 0; i < totalJudgingKeysCount; i++)
            opinions[0].first = publicKeys[i];

        // parse opinions
//        for (const auto& opinion: transactionInfo.m_opinions) {
//            auto judgingKeyIter = std::find(publicKeys.begin(), publicKeys.end()-judgedKeysCount, opinion.m_replicatorKey);
//            const auto judgingKeyIndex = judgingKeyIter - publicKeys.begin();
//
//            for (const auto& layout: opinion.m_downloadLayout) {
//                auto judgedKeyIter = std::find(publicKeys.begin()+judgingKeysCount, publicKeys.end(), opinion.m_replicatorKey);
//                const auto judgedKeyIndex = publicKeys.end() - judgedKeyIter;
//
//                auto presentOpinionIndex = judgingKeyIndex * totalJudgedKeysCount + judgedKeyIndex;
//                presentOpinionsBitset[presentOpinionIndex] = true;
//
//                auto opinionIter = std::find(opinions.begin(), opinions.end(), opinion.m_replicatorKey);
//                opinionIter->second.emplace_back(layout.m_uploadedBytes);
//            }
//        }

        // convert presentOpinionsBitset to uint8_t vector
        std::vector<uint8_t> presentOpinions;
        boost::to_block_range(presentOpinionsBitset, std::back_inserter(presentOpinions));

        // convert opinions paired vector to uint64_t vector
        std::vector<uint64_t> flatOpinions;
        for (const auto& pairedOpinions : opinions)
            for (const auto& opinion : pairedOpinions.second)
                flatOpinions.emplace_back(opinion);

        builders::DownloadApprovalBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
        builder.setDownloadChannelId(transactionInfo.m_downloadChannelId);
        builder.setSequenceNumber(sequenceNumber);
        builder.setResponseToFinishDownloadTransaction(false); // TODO set right value
        builder.setJudgingKeysCount(judgingKeysCount);
        builder.setOverlappingKeysCount(overlappingKeysCount);
        builder.setJudgedKeysCount(judgedKeysCount);
        builder.setPublicKeys(publicKeys);
        builder.setSignatures(signatures);
        builder.setPresentOpinions(presentOpinions);
        builder.setOpinions(flatOpinions);

        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
		auto transactionHash = model::CalculateHash(*pTransaction, m_generationHash);
        send(pTransaction);

        return transactionHash;
    }

    void TransactionSender::send(std::shared_ptr<model::Transaction> pTransaction) {
		extensions::TransactionExtensions(m_generationHash).sign(m_keyPair, *pTransaction);
        auto range = model::TransactionRange::FromEntity(pTransaction);
        m_transactionRangeHandler({std::move(range), m_keyPair.publicKey()});
    }
}}
