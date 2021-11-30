/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <numeric>
#include "ReplicatorEventHandler.h"

namespace catapult { namespace storage {
    void ReplicatorEventHandler::modifyTransactionEndedWithError(
                sirius::drive::Replicator& replicator,
                const sirius::Key& driveKey,
                const sirius::drive::ModifyRequest& modifyRequest,
                const std::string& reason, int errorCode) {
        CATAPULT_LOG(warning) << "modify transaction " << modifyRequest.m_transactionHash
                              << " for drive " << driveKey
                              << " finished with error (" << errorCode << ") " << reason;
    }

    void ReplicatorEventHandler::modifyApprovalTransactionIsReady(sirius::drive::Replicator& replicator, sirius::drive::ApprovalTransactionInfo&& transactionInfo) {
        CATAPULT_LOG(debug) << "modify approval transaction for " << transactionInfo.m_modifyTransactionHash.data() << " is ready";

        m_transactionSender.sendDataModificationApprovalTransaction(
                (const crypto::KeyPair&) replicator.keyPair(),
                transactionInfo
        );
        replicator.asyncApprovalTransactionHasBeenPublished(transactionInfo);
    }

    void ReplicatorEventHandler::singleModifyApprovalTransactionIsReady(sirius::drive::Replicator& replicator, sirius::drive::ApprovalTransactionInfo&& transactionInfo) {
        CATAPULT_LOG(debug) << "single modify approval transaction for " << transactionInfo.m_modifyTransactionHash.data() << " is ready";

        m_transactionSender.sendDataModificationSingleApprovalTransaction(
                (const crypto::KeyPair&) replicator.keyPair(),
                transactionInfo
        );
        replicator.asyncSingleApprovalTransactionHasBeenPublished(transactionInfo);
    }

    void ReplicatorEventHandler::downloadApprovalTransactionIsReady(sirius::drive::Replicator& replicator, const sirius::drive::DownloadApprovalTransactionInfo& info) {
        CATAPULT_LOG(debug) << "download approval transaction for" << info.m_blockHash.data() << " is ready";

        m_transactionSender.sendDownloadApprovalTransaction((const crypto::KeyPair&) replicator.keyPair(), info);
        replicator.asyncDownloadApprovalTransactionHasBeenPublished(info.m_blockHash, info.m_downloadChannelId);
    }

    void ReplicatorEventHandler::opinionHasBeenReceived(sirius::drive::Replicator& replicator, const sirius::drive::ApprovalTransactionInfo& info) {
        if (info.m_opinions.size() != 1)
            return;

        const auto& opinion = info.m_opinions.at(0);
        auto isValid = opinion.Verify(
                replicator.keyPair(),
                info.m_driveKey,
                info.m_modifyTransactionHash,
                info.m_rootHash,
                info.m_fsTreeFileSize,
                info.m_metaFilesSize,
                info.m_driveSize);

        if (!isValid) {
            CATAPULT_LOG(warning) << "failed verification";
            return;
        }

        const auto* pDriveEntry = m_storageState.getDrive(info.m_driveKey, m_serviceState.cache());
        if (!pDriveEntry)
            return;

        const auto& replicators = pDriveEntry->replicators();
        auto it = replicators.find(opinion.m_replicatorKey);
        if (it == replicators.end())
            return;

        auto actualSum = opinion.m_clientUploadBytes;
        auto expectedSum = m_storageState.getReplicatorDriveInfo(opinion.m_replicatorKey, info.m_driveKey, m_serviceState.cache())->LastCompletedCumulativeDownloadWork;

        // TODO check also offborded/excluded replicators
        for (const auto& layout : opinion.m_uploadLayout) {
            auto repIt = replicators.find(layout.m_key);
            if (repIt == replicators.end() && *repIt != pDriveEntry->owner())
                return;

            actualSum += layout.m_uploadedBytes;
        }

        auto modificationIt = std::find_if(
                pDriveEntry->activeDataModifications().begin(),
                pDriveEntry->activeDataModifications().end(),
                [&info](const auto& item){return item.Id == info.m_modifyTransactionHash;}
        );

        if (modificationIt == pDriveEntry->activeDataModifications().end())
            return;

        modificationIt++;
        expectedSum += std::accumulate(
                pDriveEntry->activeDataModifications().begin(),
                modificationIt,
                0,
                [](int64_t accumulator, const auto& currentModification){
                    return accumulator + currentModification.ActualUploadSize;
                }
        );

        // TODO compare current cumulativeSizes with exist ones (for each current should be grater or equal then exist)
        if (actualSum != expectedSum)
            return;

        replicator.asyncOnOpinionReceived(info);
    }

    void ReplicatorEventHandler::downloadOpinionHasBeenReceived(sirius::drive::Replicator& replicator, const sirius::drive::DownloadApprovalTransactionInfo& info){
        if (info.m_opinions.size() != 1)
            return;

        const auto& opinion = info.m_opinions.at(0);
        if (!opinion.Verify(info.m_blockHash, info.m_downloadChannelId)) {
            CATAPULT_LOG(warning) << "failed verification";
            return;
        }

        if (!m_storageState.isReplicatorRegistered(opinion.m_replicatorKey)){
            CATAPULT_LOG(warning) << "replicator not registered";
            return;
        }

        replicator.asyncOnDownloadOpinionReceived(info);
    }
}}
