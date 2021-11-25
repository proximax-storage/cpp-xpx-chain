/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

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
        CATAPULT_LOG(debug) << "modify approval transaction for " << transactionInfo.m_modifyTransactionHash << " is ready";

        m_transactionSender.sendDataModificationApprovalTransaction(
                (const crypto::KeyPair&) replicator.keyPair(),
                transactionInfo
        );
        replicator.asyncApprovalTransactionHasBeenPublished(transactionInfo);
    }

    void ReplicatorEventHandler::singleModifyApprovalTransactionIsReady(sirius::drive::Replicator& replicator, sirius::drive::ApprovalTransactionInfo&& transactionInfo) {
        CATAPULT_LOG(debug) << "single modify approval transaction for " << transactionInfo.m_modifyTransactionHash
                            << " is ready";

        m_transactionSender.sendDataModificationSingleApprovalTransaction(
                (const crypto::KeyPair&) replicator.keyPair(),
                transactionInfo
        );
        replicator.asyncSingleApprovalTransactionHasBeenPublished(transactionInfo);
    }

    void ReplicatorEventHandler::downloadApprovalTransactionIsReady(sirius::drive::Replicator& replicator, const sirius::drive::DownloadApprovalTransactionInfo& info) {
        CATAPULT_LOG(debug) << "download approval transaction for" << info.m_blockHash << " is ready";

        m_transactionSender.sendDownloadApprovalTransaction((const crypto::KeyPair&) replicator.keyPair(), info);
        replicator.asyncDownloadApprovalTransactionHasBeenPublished(info.m_blockHash, info.m_downloadChannelId);
    }
}}
