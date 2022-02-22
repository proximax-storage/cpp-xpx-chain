/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionSender.h"
#include "sdk/src/builders/DataModificationApprovalBuilder.h"
#include "sdk/src/builders/DataModificationSingleApprovalBuilder.h"
#include "sdk/src/builders/DownloadApprovalBuilder.h"
#include "sdk/src/builders/EndDriveVerificationBuilder.h"
#include "sdk/src/extensions/TransactionExtensions.h"
#include "catapult/model/EntityHasher.h"
#include <boost/dynamic_bitset.hpp>

namespace catapult { namespace storage {

    Hash256 TransactionSender::sendDataModificationApprovalTransaction(const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending data modification approval transaction initiated by " << Hash256(transactionInfo.m_modifyTransactionHash);
        utils::KeySet judgedKeys;
		utils::KeySet judgingKeys;
        std::vector<Key> overlappingKeys;
		std::map<Key, std::pair<uint64_t, std::map<Key, uint64_t>>> opinionMap;
		uint16_t opinionCount = 0u;
		std::map<Key, Signature> signatureMap;
		bool hasClientUploads = false;

        // collect judging and judged keys, opinions and signatures.
        for (const auto& opinion : transactionInfo.m_opinions) {
			if (opinion.m_uploadLayout.empty())
				continue;

            judgingKeys.emplace(opinion.m_replicatorKey);
			signatureMap[opinion.m_replicatorKey] = opinion.m_signature.array();
			auto& pair = opinionMap[opinion.m_replicatorKey];
            for (const auto& layout: opinion.m_uploadLayout) {
				judgedKeys.emplace(layout.m_key);
				pair.second[layout.m_key] = layout.m_uploadedBytes;
				opinionCount++;
			}
        }

		// separate overlapping keys
		auto& set1 = judgedKeys.size() < judgingKeys.size() ? judgedKeys : judgingKeys;
		auto& set2 = judgedKeys.size() >= judgingKeys.size() ? judgedKeys : judgingKeys;
		for (auto iter1 = set1.begin(); iter1 != set1.end();) {
			auto iter2 = set2.find(*iter1);
			if (iter2 != set2.end()) {
				overlappingKeys.push_back(*iter1);
				iter1 = set1.erase(iter1);
				set2.erase(iter2);
			} else {
				iter1++;
			}
		}

        // merge all keys
        std::vector<Key> publicKeys;
		auto totalKeyCount = judgingKeys.size() + overlappingKeys.size() + judgedKeys.size();
		publicKeys.reserve(totalKeyCount + 1);
        publicKeys.insert(publicKeys.end(), judgingKeys.begin(), judgingKeys.end());
        publicKeys.insert(publicKeys.end(), overlappingKeys.begin(), overlappingKeys.end());
        publicKeys.insert(publicKeys.end(), judgedKeys.begin(), judgedKeys.end());
		auto totalJudgingKeysCount = judgingKeys.size() + overlappingKeys.size();
		auto totalJudgedKeysCount = overlappingKeys.size() + judgedKeys.size();
		if (hasClientUploads) {
			publicKeys.push_back(m_storageState.getDrive(transactionInfo.m_driveKey).Owner);
			totalKeyCount++;
			totalJudgedKeysCount++;
		}

		// prepare opinions
        boost::dynamic_bitset<uint8_t> presentOpinionsBitset(totalJudgingKeysCount * totalJudgedKeysCount, 0u);
		std::vector<uint64_t> opinions;
		opinions.reserve(opinionCount);
		std::vector<Signature> signatures;
		signatures.reserve(transactionInfo.m_opinions.size());
		auto judgingKeysOffset = totalKeyCount - totalJudgedKeysCount;
		auto replicatorJudgedKeysCount = totalJudgedKeysCount - (hasClientUploads ? 1 : 0);
		for (auto i = 0u; i < totalJudgingKeysCount; ++i) {
			signatures.push_back(signatureMap.at(publicKeys[i]));
			const auto& opinion = opinionMap.at(publicKeys[i]);
			for (auto k = 0u; k < replicatorJudgedKeysCount; ++k) {
				auto iter = opinion.second.find(publicKeys[k + judgingKeysOffset]);
				if (iter != opinion.second.end()) {
					presentOpinionsBitset[i * totalJudgedKeysCount + k] = 1;
					opinions.push_back(iter->second);
				}
			}
			if (opinion.first > 0) {
				presentOpinionsBitset[i * totalJudgedKeysCount + replicatorJudgedKeysCount] = 1;
				opinions.push_back(opinion.first);
			}
		}
		std::vector<uint8_t> presentOpinions;
		presentOpinions.reserve((totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8);
		boost::to_block_range(presentOpinionsBitset, std::back_inserter(presentOpinions));

		// build and send transaction
        builders::DataModificationApprovalBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
        builder.setDriveKey(transactionInfo.m_driveKey);
        builder.setDataModificationId(transactionInfo.m_modifyTransactionHash);
        builder.setFileStructureCdi(transactionInfo.m_rootHash);
        builder.setFileStructureSize(transactionInfo.m_fsTreeFileSize);
        builder.setMetaFilesSize(transactionInfo.m_metaFilesSize);
        builder.setUsedDriveSize(transactionInfo.m_driveSize);
        builder.setJudgingKeysCount(judgingKeys.size());
        builder.setOverlappingKeysCount(overlappingKeys.size());
        builder.setJudgedKeysCount(judgedKeys.size() + (hasClientUploads ? 1 : 0));
        builder.setPublicKeys(std::move(publicKeys));
        builder.setSignatures(std::move(signatures));
        builder.setPresentOpinions(std::move(presentOpinions));
        builder.setOpinions(std::move(opinions));
        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
        send(pTransaction);

        return model::CalculateHash(*pTransaction, m_generationHash);
    }

    Hash256 TransactionSender::sendDataModificationSingleApprovalTransaction(const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending data modification single approval transaction initiated by " << Hash256(transactionInfo.m_modifyTransactionHash);

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
        builder.setPublicKeys(std::move(keys));
        builder.setOpinions(std::move(opinions));

        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
        send(pTransaction);

        return model::CalculateHash(*pTransaction, m_generationHash);
    }

    Hash256 TransactionSender::sendDownloadApprovalTransaction(
            uint16_t sequenceNumber,
            const sirius::drive::DownloadApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending download approval transaction initiated by " << Hash256(transactionInfo.m_blockHash);
        utils::KeySet judgedKeys;
		utils::KeySet judgingKeys;
        std::vector<Key> overlappingKeys;
		std::map<Key, std::map<Key, uint64_t>> opinionMap;
		uint16_t opinionCount = 0u;
		std::map<Key, Signature> signatureMap;

        // collect judging and judged keys, opinions and signatures.
        for (const auto& opinion : transactionInfo.m_opinions) {
			if (opinion.m_downloadLayout.empty())
				continue;

            judgingKeys.emplace(opinion.m_replicatorKey);
			signatureMap[opinion.m_replicatorKey] = opinion.m_signature.array();
            for (const auto& layout: opinion.m_downloadLayout) {
				judgedKeys.emplace(layout.m_key);
				opinionMap[opinion.m_replicatorKey][layout.m_key] = layout.m_uploadedBytes;
				opinionCount++;
			}
        }

		// separate overlapping keys
		auto& set1 = judgedKeys.size() < judgingKeys.size() ? judgedKeys : judgingKeys;
		auto& set2 = judgedKeys.size() >= judgingKeys.size() ? judgedKeys : judgingKeys;
		for (auto iter1 = set1.begin(); iter1 != set1.end();) {
			auto iter2 = set2.find(*iter1);
			if (iter2 != set2.end()) {
				overlappingKeys.push_back(*iter1);
				iter1 = set1.erase(iter1);
				set2.erase(iter2);
			} else {
				iter1++;
			}
		}

        // merge all keys
        std::vector<Key> publicKeys;
		auto totalKeyCount = judgingKeys.size() + overlappingKeys.size() + judgedKeys.size();
		publicKeys.reserve(totalKeyCount);
        publicKeys.insert(publicKeys.end(), judgingKeys.begin(), judgingKeys.end());
        publicKeys.insert(publicKeys.end(), overlappingKeys.begin(), overlappingKeys.end());
        publicKeys.insert(publicKeys.end(), judgedKeys.begin(), judgedKeys.end());
		auto totalJudgingKeysCount = judgingKeys.size() + overlappingKeys.size();
		auto totalJudgedKeysCount = overlappingKeys.size() + judgedKeys.size();

		// prepare opinions
        boost::dynamic_bitset<uint8_t> presentOpinionsBitset(totalJudgingKeysCount * totalJudgedKeysCount, 0u);
		std::vector<uint64_t> opinions;
		opinions.reserve(opinionCount);
		std::vector<Signature> signatures;
		signatures.reserve(transactionInfo.m_opinions.size());
		auto judgingKeysOffset = totalKeyCount - totalJudgedKeysCount;
		for (auto i = 0u; i < totalJudgingKeysCount; ++i) {
			signatures.push_back(signatureMap.at(publicKeys[i]));
			const auto& opinion = opinionMap.at(publicKeys[i]);
			for (auto k = 0u; k < totalJudgedKeysCount; ++k) {
				auto iter = opinion.find(publicKeys[k + judgingKeysOffset]);
				if (iter != opinion.end()) {
					presentOpinionsBitset[i * totalJudgedKeysCount + k] = 1;
					opinions.push_back(iter->second);
				}
			}
		}
		std::vector<uint8_t> presentOpinions;
		presentOpinions.reserve((totalJudgingKeysCount * totalJudgedKeysCount + 7) / 8);
		boost::to_block_range(presentOpinionsBitset, std::back_inserter(presentOpinions));

		// build and send transaction
        builders::DownloadApprovalBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
        builder.setDownloadChannelId(transactionInfo.m_downloadChannelId);
		builder.setApprovalTrigger(transactionInfo.m_blockHash);
        builder.setSequenceNumber(sequenceNumber);
        builder.setResponseToFinishDownloadTransaction(false); // TODO set right value
        builder.setJudgingKeysCount(judgingKeys.size());
        builder.setOverlappingKeysCount(overlappingKeys.size());
        builder.setJudgedKeysCount(judgedKeys.size());
        builder.setPublicKeys(std::move(publicKeys));
        builder.setSignatures(std::move(signatures));
        builder.setPresentOpinions(std::move(presentOpinions));
        builder.setOpinions(std::move(opinions));
        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
        send(pTransaction);

        return model::CalculateHash(*pTransaction, m_generationHash);
    }

    Hash256 TransactionSender::sendEndDriveVerificationTransaction(const sirius::drive::VerifyApprovalTxInfo& transactionInfo) {
		uint8_t opinionCount = transactionInfo.m_opinions[0].m_opinions.size();
		uint8_t keyCount = opinionCount + 1;
		uint8_t judgingKeyCount = transactionInfo.m_opinions.size();
		std::vector<Key> publicKeys;
		publicKeys.reserve(keyCount);
		std::vector<Signature> signatures;
		signatures.reserve(judgingKeyCount);
		boost::dynamic_bitset<uint8_t> opinionsBitset(judgingKeyCount * opinionCount);

		for (auto i = 0u; i < transactionInfo.m_opinions.size(); ++i) {
			const auto& opinion = transactionInfo.m_opinions[i];
			publicKeys.emplace_back(opinion.m_publicKey);
			signatures.emplace_back(opinion.m_signature.array());
			for (auto k = 0u; k < opinionCount; ++k)
				opinionsBitset[i * opinionCount + k] = opinion.m_opinions[k];
		}

		std::vector<uint8_t> opinions;
		opinions.reserve((judgingKeyCount * opinionCount + 7) / 8);
		boost::to_block_range(opinionsBitset, std::back_inserter(opinions));

		// build and send transaction
        builders::EndDriveVerificationBuilder builder(m_networkIdentifier, m_keyPair.publicKey());
        builder.setDriveKey(transactionInfo.m_driveKey);
		builder.setVerificationTrigger(transactionInfo.m_tx);
        builder.setShardId(transactionInfo.m_shardId);
        builder.setKeyCount(keyCount);
        builder.setJudgingKeyCount(judgingKeyCount);
        builder.setPublicKeys(std::move(publicKeys));
        builder.setSignatures(std::move(signatures));
        builder.setOpinions(std::move(opinions));
        auto pTransaction = utils::UniqueToShared(builder.build());
        pTransaction->Deadline = utils::NetworkTime() + Timestamp(m_storageConfig.TransactionTimeout.millis());
        send(pTransaction);

        return model::CalculateHash(*pTransaction, m_generationHash);
	}

    void TransactionSender::send(std::shared_ptr<model::Transaction> pTransaction) {
		extensions::TransactionExtensions(m_generationHash).sign(m_keyPair, *pTransaction);
        auto range = model::TransactionRange::FromEntity(pTransaction);
        m_transactionRangeHandler({std::move(range), m_keyPair.publicKey()});
    }
}}
