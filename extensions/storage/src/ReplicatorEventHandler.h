/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "drive/Replicator.h"
#include "TransactionSender.h"

namespace catapult { namespace storage {
    class ReplicatorEventHandler: public sirius::drive::ReplicatorEventHandler {
    public:
        explicit ReplicatorEventHandler(TransactionSender& transactionSender) : m_transactionSender(transactionSender) {}

    public:
        void modifyTransactionEndedWithError(sirius::drive::Replicator& replicator,
                                            const sirius::Key& driveKey,
                                            const sirius::drive::ModifyRequest& modifyRequest,
                                            const std::string& reason, int errorCode) override;

        void modifyApprovalTransactionIsReady(sirius::drive::Replicator& replicator, sirius::drive::ApprovalTransactionInfo&& transactionInfo) override;

        void singleModifyApprovalTransactionIsReady(sirius::drive::Replicator& replicator, sirius::drive::ApprovalTransactionInfo&& transactionInfo) override;

        void downloadApprovalTransactionIsReady(sirius::drive::Replicator& replicator, const sirius::drive::DownloadApprovalTransactionInfo& info) override;

    private:
        TransactionSender m_transactionSender;
    };
}}
