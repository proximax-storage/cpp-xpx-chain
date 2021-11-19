/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "TransactionSender.h"
#include "sdk/src/builders/DataModificationApprovalBuilder.h"
#include "sdk/src/builders/DataModificationSingleApprovalBuilder.h"
#include "catapult/extensions/TransactionExtensions.h"

namespace catapult { namespace storage {
    void TransactionSender::sendDataModificationApprovalTransaction(const crypto::KeyPair& sender,
                                                                    const sirius::drive::ApprovalTransactionInfo& transactionInfo) {
        CATAPULT_LOG(debug) << "sending data modification approval transaction " << transactionInfo.m_modifyTransactionHash;

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
        CATAPULT_LOG(debug) << "sending data modification single approval transaction " << transactionInfo.m_modifyTransactionHash;

        builders::DataModificationSingleApprovalBuilder builder(m_networkIdentifier, sender.publicKey());
        builder.setDriveKey(transactionInfo.m_driveKey);
        builder.setDataModificationId(transactionInfo.m_modifyTransactionHash);
        builder.setUsedDriveSize(transactionInfo.m_driveSize);
//        builder.setUploaderKeys();
//        builder.setUploadOpinion();

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
