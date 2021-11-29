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
//        CATAPULT_LOG(warning) << "modify transaction " << modifyRequest.m_transactionHash
//                              << " for drive " << driveKey
//                              << " finished with error (" << errorCode << ") " << reason;
    }

    void ReplicatorEventHandler::modifyApprovalTransactionIsReady(sirius::drive::Replicator& replicator, sirius::drive::ApprovalTransactionInfo&& transactionInfo) {
//        CATAPULT_LOG(debug) << "modify approval transaction for " << transactionInfo.m_modifyTransactionHash << " is ready";

        m_transactionSender.sendDataModificationApprovalTransaction(
                (const crypto::KeyPair&) replicator.keyPair(),
                transactionInfo
        );
        replicator.asyncApprovalTransactionHasBeenPublished(transactionInfo);
    }

    void ReplicatorEventHandler::singleModifyApprovalTransactionIsReady(sirius::drive::Replicator& replicator, sirius::drive::ApprovalTransactionInfo&& transactionInfo) {
//        CATAPULT_LOG(debug) << "single modify approval transaction for " << transactionInfo.m_modifyTransactionHash
//                            << " is ready";

        m_transactionSender.sendDataModificationSingleApprovalTransaction(
                (const crypto::KeyPair&) replicator.keyPair(),
                transactionInfo
        );
        replicator.asyncSingleApprovalTransactionHasBeenPublished(transactionInfo);
    }

    void ReplicatorEventHandler::downloadApprovalTransactionIsReady(sirius::drive::Replicator& replicator, const sirius::drive::DownloadApprovalTransactionInfo& info) {
//        CATAPULT_LOG(debug) << "download approval transaction for" << info.m_blockHash << " is ready";

        m_transactionSender.sendDownloadApprovalTransaction((const crypto::KeyPair&) replicator.keyPair(), info);
        replicator.asyncDownloadApprovalTransactionHasBeenPublished(info.m_blockHash, info.m_downloadChannelId);
    }

    void ReplicatorEventHandler::opinionHasBeenReceived(sirius::drive::Replicator& replicator, const sirius::drive::ApprovalTransactionInfo& info) {
        if (info.m_opinions.size() != 1)
            return;

        const auto& opinion = info.m_opinions.at(0);
        if (!opinion.Verify(info.m_modifyTransactionHash, info.m_rootHash)) {
            CATAPULT_LOG(warning) << "failed verification";
            return;
        }

        const auto* pDriveEntry = m_storageState.getDrive(info.m_driveKey);
        if (!pDriveEntry)
            return;

        const auto& replicators = pDriveEntry->replicators();
        auto it = replicators.find(opinion.m_replicatorKey);
        if (it == replicators.end())
            return;

        auto actualSum = std::accumulate(opinion.m_replicatorUploadBytes.begin(), opinion.m_replicatorUploadBytes.end(), opinion.m_clientUploadBytes);
        auto expectedSum = m_storageState.getReplicatorDriveInfo(opinion.m_replicatorKey, info.m_driveKey);

        if (pDriveEntry->completedDataModifications().empty()
            || pDriveEntry->completedDataModifications.back().Id != info.m_modifyTransactionHash) {
            auto modificationIt = std::find_if(
                    pDriveEntry->activeDataModification.begin(),
                    pDriveEntry->activeDataModification.end(),
                    [&info](const auto& item){return item.Id == info.m_modifyTransactionHash;}
            );

            if (modificationIt == pDriveEntry->activeDataModification.end())
                return;

            modificationIt++;
            expectedSum += std::accumulate(pDriveEntry->activeDataModification.begin(), modificationIt, 0, [](const auto& currentSum, const auto& currentModification){
                return currentSum + currentModification.ActualUploadSize;
            });
        }

        // TODO compare current cumulativeSizes with exist ones (for each current should be grater or equal then exist)
        if (actualSum != expectedSum)
            return;

        // TODO check also offborded/excluded replicators
        for (auto it = opinion.m_uploadReplicatorKeys.begin(); it != opinion.m_uploadReplicatorKeys.end(); it += sizeof(Key)) {
            Key* key = (catapult::Key*) &(*it);
            auto repIt = replicators.find(*key);
            if (repIt == replicators.end() && *repIt != pDriveEntry.Owner)
                return;
        }

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

        replicator.asyncOnDownloadOpinionReceived(info);
    }
}}
