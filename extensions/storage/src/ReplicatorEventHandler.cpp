/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorEventHandler.h"
#include "TransactionSender.h"
#include "TransactionStatusHandler.h"
#include "plugins/txes/storage/src/validators/Results.h"
#include <numeric>
#include <iostream>

namespace catapult { namespace storage {

    namespace {
        class DefaultReplicatorEventHandler : public ReplicatorEventHandler {
        public:
            explicit DefaultReplicatorEventHandler(
                    TransactionSender&& transactionSender,
                    state::StorageState& storageState,
                    TransactionStatusHandler& transactionStatusHandler)
				: m_transactionSender(std::move(transactionSender))
				, m_storageState(storageState)
				, m_transactionStatusHandler(transactionStatusHandler)
			{}

        public:
            void modifyApprovalTransactionIsReady(
                    sirius::drive::Replicator&,
                    const sirius::drive::ApprovalTransactionInfo& transactionInfo) override {
				auto pReplicator = m_pReplicator.lock();
				if (!pReplicator)
					return;

				CATAPULT_LOG(debug) << "sending data modification approval transaction";

                auto transactionHash = m_transactionSender.sendDataModificationApprovalTransaction(transactionInfo);

				m_transactionStatusHandler.addHandler(transactionHash, [
						transactionHash,
						driveKey = transactionInfo.m_driveKey,
						pReplicatorWeak = m_pReplicator](uint32_t status) {
					auto pReplicator = pReplicatorWeak.lock();
					if (!pReplicator)
						return;

					auto validationResult = validators::ValidationResult(status);
					CATAPULT_LOG(debug) << "data modification approval transaction completed with " << validationResult;
                    if (validationResult == validators::Failure_Storage_Opinion_Invalid_Key)
                    	pReplicator->asyncApprovalTransactionHasFailedInvalidOpinions(driveKey, transactionHash.array());
                });
            }

            void singleModifyApprovalTransactionIsReady(
                    sirius::drive::Replicator&,
                    const sirius::drive::ApprovalTransactionInfo& transactionInfo) override {
				auto pReplicator = m_pReplicator.lock();
				if (!pReplicator)
					return;

				CATAPULT_LOG(debug) << "sending data modification single approval transaction";

                auto transactionHash = m_transactionSender.sendDataModificationSingleApprovalTransaction(transactionInfo);
            }

            void downloadApprovalTransactionIsReady(
                    sirius::drive::Replicator&,
                    const sirius::drive::DownloadApprovalTransactionInfo& info) override {
				auto pReplicator = m_pReplicator.lock();
				if (!pReplicator)
					return;

				CATAPULT_LOG(debug) << "sending download approval transaction";

				auto transactionHash = m_transactionSender.sendDownloadApprovalTransaction(info);

				m_transactionStatusHandler.addHandler(transactionHash, [
						blockHash = info.m_blockHash,
						downloadChannelId = info.m_downloadChannelId,
						pReplicatorWeak = m_pReplicator](uint32_t status) {
					auto pReplicator = pReplicatorWeak.lock();
					if (!pReplicator)
						return;

					auto validationResult = validators::ValidationResult(status);
					CATAPULT_LOG(debug) << "download approval transaction completed with " << validationResult;
                    if (validationResult != validators::Failure_Storage_Invalid_Approval_Trigger)
                        pReplicator->asyncDownloadApprovalTransactionHasFailedInvalidOpinions(blockHash, downloadChannelId);
                });
            }

            void verificationTransactionIsReady(
                    sirius::drive::Replicator&,
					const sirius::drive::VerifyApprovalTxInfo& transactionInfo) override {
                CATAPULT_LOG(debug) << "sending end drive verification transaction";

				auto pReplicator = m_pReplicator.lock();
				if (!pReplicator)
					return;

                auto transactionHash = m_transactionSender.sendEndDriveVerificationTransaction(transactionInfo);

				m_transactionStatusHandler.addHandler(transactionHash, [
						transactionHash,
						driveKey = transactionInfo.m_driveKey,
						pReplicatorWeak = m_pReplicator](uint32_t status) {
					auto pReplicator = pReplicatorWeak.lock();
					if (!pReplicator)
						return;

					auto validationResult = validators::ValidationResult(status);
					CATAPULT_LOG(debug) << "end drive verification transaction completed with " << validationResult;
                    if (validationResult == validators::ValidationResult::Success) {
                        pReplicator->asyncVerifyApprovalTransactionHasBeenPublished(sirius::drive::PublishedVerificationApprovalTransactionInfo{
							transactionHash.array(),
							driveKey});
                    } else if (validationResult == validators::Failure_Storage_Opinion_Invalid_Key) {
						pReplicator->asyncVerifyApprovalTransactionHasFailedInvalidOpinions(driveKey, transactionHash.array());
					}
                });
            }

            void opinionHasBeenReceived(
                    sirius::drive::Replicator&,
                    const sirius::drive::ApprovalTransactionInfo& info) override {
				CATAPULT_LOG(debug) << "opinionHasBeenReceived() " << int(info.m_opinions[0].m_replicatorKey[0]);
				auto pReplicator = m_pReplicator.lock();
				if (!pReplicator)
					return;

//                if (!m_storageState.driveExist(info.m_driveKey))
//                    return;
//
//                if (info.m_opinions.size() != 1)
//                    return;
//
//                const auto& opinion = info.m_opinions.at(0);
//                auto isValid = opinion.Verify(
//                        pReplicator->keyPair(),
//                        info.m_driveKey,
//                        info.m_modifyTransactionHash,
//                        info.m_rootHash,
//                        info.m_fsTreeFileSize,
//                        info.m_metaFilesSize,
//                        info.m_driveSize);
//
//                if (!isValid) {
//                    CATAPULT_LOG(warning) << "failed verification";
//                    return;
//                }
//
//                const auto pDriveEntry = m_storageState.getDrive(info.m_driveKey);
//                const auto& replicators = pDriveEntry.Replicators;
//                auto it = replicators.find(opinion.m_replicatorKey);
//                if (it == replicators.end())
//                    return;
//
//                if (pDriveEntry.DataModifications.empty())
//                    return;
//
//                auto actualSum = opinion.m_clientUploadBytes;
//                auto expectedSum = m_storageState.getDownloadWork(opinion.m_replicatorKey, info.m_driveKey);
//
//                // TODO check also offboarded/excluded replicators
//                for (const auto& layout: opinion.m_uploadLayout) {
//                    auto repIt = replicators.find(layout.m_key);
//                    if (repIt == replicators.end() && *repIt != pDriveEntry.Owner)
//                        return;
//
//                    actualSum += layout.m_uploadedBytes;
//                }
//
//                auto modificationIt = std::find_if(
//                        pDriveEntry.DataModifications.begin(),
//                        pDriveEntry.DataModifications.end(),
//                        [&info](const auto& item) { return item.Id == info.m_modifyTransactionHash; }
//                );
//
//                if (modificationIt == pDriveEntry.DataModifications.end())
//                    return;
//
//                modificationIt++;
//                expectedSum += std::accumulate(
//                        pDriveEntry.DataModifications.begin(),
//                        modificationIt,
//                        0,
//                        [](int64_t accumulator, const auto& currentModification) {
//                            return accumulator + currentModification.ActualUploadSize;
//                        }
//                );
//
//                // TODO compare current cumulativeSizes with exist ones (for each current should be grater or equal then exist)
//                if (actualSum != expectedSum)
//                    return;

                pReplicator->asyncOnOpinionReceived(info);
            }

            void downloadOpinionHasBeenReceived(
                    sirius::drive::Replicator&,
                    const sirius::drive::DownloadApprovalTransactionInfo& info) override {
				auto pReplicator = m_pReplicator.lock();
				if (!pReplicator)
					return;

//                if (!m_storageState.downloadChannelExist(info.m_downloadChannelId))
//                    return;
//
//                if (info.m_opinions.size() != 1)
//                    return;
//
//                const auto& opinion = info.m_opinions.at(0);
//                if (!opinion.Verify(info.m_blockHash, info.m_downloadChannelId)) {
//                    CATAPULT_LOG(warning) << "failed verification";
//                    return;
//                }
//
//                if (!m_storageState.isReplicatorRegistered(opinion.m_replicatorKey)) {
//                    CATAPULT_LOG(warning) << "replicator not registered";
//                    return;
//                }

                pReplicator->asyncOnDownloadOpinionReceived(info);
            }

        private:
            TransactionSender m_transactionSender;
            state::StorageState& m_storageState;
            TransactionStatusHandler& m_transactionStatusHandler;
        };
    }

    std::unique_ptr<ReplicatorEventHandler> CreateReplicatorEventHandler(
            TransactionSender&& transactionSender,
            state::StorageState& storageState,
            TransactionStatusHandler& operations) {
        return std::make_unique<DefaultReplicatorEventHandler>(std::move(transactionSender), storageState, operations);
    }
}}
