/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <numeric>
#include "ReplicatorEventHandler.h"
#include "plugins/txes/storage/src/validators/Results.h"

namespace catapult { namespace storage {

	namespace {
		class ReplicatorEventHandler: public sirius::drive::ReplicatorEventHandler {
		public:
			explicit ReplicatorEventHandler(
                    TransactionSender&& transactionSender,
                    state::StorageState& storageState,
                    TransactionStatusHandler& transactionStatusHandler)
				: m_transactionSender(std::move(transactionSender))
				, m_storageState(storageState)
				, m_transactionStatusHandler(transactionStatusHandler)
			{}

		public:
			void modifyTransactionEndedWithError(
					sirius::drive::Replicator& replicator,
					const sirius::Key& driveKey,
					const sirius::drive::ModifyRequest& modifyRequest,
					const std::string& reason, int errorCode) override {
				CATAPULT_LOG(warning) << "modify transaction " << modifyRequest.m_transactionHash
									  << " for drive " << driveKey
									  << " finished with error (" << errorCode << ") " << reason;
			}

			void modifyApprovalTransactionIsReady(
					sirius::drive::Replicator& replicator,
					sirius::drive::ApprovalTransactionInfo&& transactionInfo) override {
				CATAPULT_LOG(debug) << "modify approval transaction for " << transactionInfo.m_modifyTransactionHash.data() << " is ready";
				m_transactionSender.sendDataModificationApprovalTransaction(
					(const crypto::KeyPair &) replicator.keyPair(),
					transactionInfo
				);

				auto sign = m_transactionSender.sendDataModificationApprovalTransaction((const crypto::KeyPair &) replicator.keyPair(), transactionInfo);
				Callback handler = [&](const Hash256& hash, uint32_t status){
					if (!!status)
						replicator.asyncApprovalTransactionHasBeenPublished(transactionInfo);

					validators::ValidationResult validationResult{status};
					if (validationResult != validators::Failure_Storage_No_Active_Data_Modifications
						&& validationResult != validators::Failure_Storage_Opinion_Invalid_Key)
						return;

					replicator.asyncApprovalTransactionHasFailedInvalidSignatures(
							sirius::Key{}, //TODO pass real key
							(const std::array<uint8_t, 32UL>&) hash
					);
				};

				m_transactionStatusHandler.addHandler(sign, handler);
			}

			void singleModifyApprovalTransactionIsReady(
					sirius::drive::Replicator& replicator,
					sirius::drive::ApprovalTransactionInfo&& transactionInfo) override {
				CATAPULT_LOG(debug) << "single modify approval transaction for "
									<< transactionInfo.m_modifyTransactionHash.data() << " is ready";

				m_transactionSender.sendDataModificationSingleApprovalTransaction(
					(const crypto::KeyPair &) replicator.keyPair(),
					transactionInfo
				);

				auto sign = m_transactionSender.sendDataModificationApprovalTransaction((const crypto::KeyPair &) replicator.keyPair(), transactionInfo);
				Callback handler = [&](const Hash256& hash, uint32_t status){
					if (!!status)
						replicator.asyncSingleApprovalTransactionHasBeenPublished(transactionInfo);
				};

				m_transactionStatusHandler.addHandler(sign, handler);
			}

			void downloadApprovalTransactionIsReady(
					sirius::drive::Replicator& replicator,
					const sirius::drive::DownloadApprovalTransactionInfo& info) override {
				CATAPULT_LOG(debug) << "download approval transaction for" << info.m_blockHash.data() << " is ready";

				auto sign = m_transactionSender.sendDownloadApprovalTransaction((const crypto::KeyPair &) replicator.keyPair(), info);
				Callback handler = [&](const Hash256& hash, uint32_t status){
					if (!!status)
						replicator.asyncDownloadApprovalTransactionHasBeenPublished(info.m_blockHash, info.m_downloadChannelId);

					validators::ValidationResult validationResult{status};
					if (validationResult != validators::Failure_Storage_Invalid_Sequence_Number)
						return;

					replicator.asyncDownloadApprovalTransactionHasFailedInvalidSignatures(
							(const sirius::utils::ByteArray<32, sirius::Hash256_tag>&) hash,
							info.m_downloadChannelId
					);
				};

				m_transactionStatusHandler.addHandler(sign, handler);
			}

			void opinionHasBeenReceived(
					sirius::drive::Replicator& replicator,
					const sirius::drive::ApprovalTransactionInfo& info) override {
				if (info.m_opinions.size() != 1)
					return;

				const auto &opinion = info.m_opinions.at(0);
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

				const auto pDriveEntry = m_storageState.getDrive(info.m_driveKey);
				const auto &replicators = pDriveEntry.Replicators;
				auto it = replicators.find(opinion.m_replicatorKey);
				if (it == replicators.end())
					return;

				auto actualSum = opinion.m_clientUploadBytes;
				auto expectedSum = m_storageState.getDownloadWork(opinion.m_replicatorKey, info.m_driveKey);

				// TODO check also offboarded/excluded replicators
				for (const auto &layout: opinion.m_uploadLayout) {
					auto repIt = replicators.find(layout.m_key);
					if (repIt == replicators.end() && *repIt != pDriveEntry.Owner)
						return;

					actualSum += layout.m_uploadedBytes;
				}

				auto modificationIt = std::find_if(
					pDriveEntry.DataModifications.begin(),
					pDriveEntry.DataModifications.end(),
					[&info](const auto &item) { return item.Id == info.m_modifyTransactionHash; }
				);

				if (modificationIt == pDriveEntry.DataModifications.end())
					return;

				modificationIt++;
				expectedSum += std::accumulate(
					pDriveEntry.DataModifications.begin(),
					modificationIt,
					0,
					[](int64_t accumulator, const auto &currentModification) {
						return accumulator + currentModification.ActualUploadSize;
					}
				);

				// TODO compare current cumulativeSizes with exist ones (for each current should be grater or equal then exist)
				if (actualSum != expectedSum)
					return;

				replicator.asyncOnOpinionReceived(info);
			}

			void downloadOpinionHasBeenReceived(
					sirius::drive::Replicator& replicator,
					const sirius::drive::DownloadApprovalTransactionInfo& info) override {
				if (info.m_opinions.size() != 1)
					return;

				const auto &opinion = info.m_opinions.at(0);
				if (!opinion.Verify(info.m_blockHash, info.m_downloadChannelId)) {
					CATAPULT_LOG(warning) << "failed verification";
					return;
				}

				if (!m_storageState.isReplicatorRegistered(opinion.m_replicatorKey)) {
					CATAPULT_LOG(warning) << "replicator not registered";
					return;
				}

				replicator.asyncOnDownloadOpinionReceived(info);
			}

		private:
			TransactionSender m_transactionSender;
			state::StorageState& m_storageState;
			TransactionStatusHandler& m_transactionStatusHandler;
		};
	}

	std::unique_ptr<sirius::drive::ReplicatorEventHandler> CreateReplicatorEventHandler(
            TransactionSender&& transactionSender,
            state::StorageState& storageState,
            TransactionStatusHandler& operations) {
		return std::make_unique<ReplicatorEventHandler>(std::move(transactionSender), storageState, operations);
	}
}}
