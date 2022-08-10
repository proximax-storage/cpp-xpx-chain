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
            		std::shared_ptr<thread::IoThreadPool>&& pool,
                    TransactionSender&& transactionSender,
                    state::StorageState& storageState,
                    TransactionStatusHandler& transactionStatusHandler,
                    const crypto::KeyPair& keyPair)
				: m_pool(std::move(pool))
				, m_transactionSender(std::move(transactionSender))
				, m_storageState(storageState)
				, m_transactionStatusHandler(transactionStatusHandler)
				, m_keyPair(keyPair)
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

					std::set<validators::ValidationResult> handledResults = {
						validators::Failure_Storage_Signature_Count_Insufficient,
						validators::Failure_Storage_Replicator_Not_Found,
						validators::Failure_Storage_Invalid_Opinion,
						validators::Failure_Storage_Invalid_Opinions_Sum
					};

					if (handledResults.find(validationResult) != handledResults.end())
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

					std::set<validators::ValidationResult> handledResults = {
							validators::Failure_Storage_Signature_Count_Insufficient,
							validators::Failure_Storage_Opinion_Invalid_Key
					};

					if (handledResults.find(validationResult) != handledResults.end())
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

//				m_transactionStatusHandler.addHandler(transactionHash, [
//						transactionHash,
//						driveKey = transactionInfo.m_driveKey,
//						pReplicatorWeak = m_pReplicator](uint32_t status) {
//					auto pReplicator = pReplicatorWeak.lock();
//					if (!pReplicator)
//						return;

//					auto validationResult = validators::ValidationResult(status);
//					CATAPULT_LOG(debug) << "end drive verification transaction completed with " << validationResult;
//                    if (validationResult == validators::ValidationResult::Success) {
//                        pReplicator->asyncVerifyApprovalTransactionHasBeenPublished(sirius::drive::PublishedVerificationApprovalTransactionInfo{
//							transactionHash.array(),
//							driveKey});
//                    } else if (validationResult == validators::Failure_Storage_Opinion_Invalid_Key) {
//						pReplicator->asyncVerifyApprovalTransactionHasFailedInvalidOpinions(driveKey, transactionHash.array());
//					}
//                });
            }

            void opinionHasBeenReceived(
                    sirius::drive::Replicator&,
                    const sirius::drive::ApprovalTransactionInfo& info) override {
            	boost::asio::post(m_pool->ioContext(), [this, info] {
				  CATAPULT_LOG(debug) << "modificationOpinionHasBeenReceived() " << int(info.m_opinions[0].m_replicatorKey[0]);
				  auto pReplicator = m_pReplicator.lock();
				  if (!pReplicator)
				  	return;

				  if (!m_storageState.driveExists(info.m_driveKey)) {
				  	CATAPULT_LOG(warning) << "received modification opinion for a non existing drive";
				  	return;
				  }

				  if (info.m_opinions.size() != 1) {
				  	CATAPULT_LOG(warning) << "received modification opinion with many opinions";
				  	return;
				  }

				  bool sorted = true;

				  const auto& uploadLayout = info.m_opinions[0].m_uploadLayout;
				  for (int i = 0; i < uploadLayout.size() - 1 && sorted; i++) {
				  	if (Key(uploadLayout[i + 1].m_key) < Key(uploadLayout[i].m_key)) {
				  		sorted = false;
				  	}
				  }

				  if (!sorted) {
				  	CATAPULT_LOG( error ) << "received unsorted modification opinion";
				  	return;
				  }

				  const auto& opinion = info.m_opinions.at(0);
				  auto isValid = opinion.Verify(
				  		reinterpret_cast<const sirius::crypto::KeyPair&>(m_keyPair),
				  		info.m_driveKey,
				  		info.m_modifyTransactionHash,
				  		info.m_rootHash,
				  		info.m_fsTreeFileSize,
				  		info.m_metaFilesSize,
				  		info.m_driveSize);

				  if (!isValid) {
				  	CATAPULT_LOG(warning) << "received opinion with incorrect signature";
				  	return;
				  }

				  const auto pDriveEntry = m_storageState.getDrive(info.m_driveKey);
				  const auto& replicators = pDriveEntry.Replicators;
				  auto it = replicators.find(opinion.m_replicatorKey);
				  if (it == replicators.end()) {
				  	CATAPULT_LOG( warning ) << "received opinion from not a replicator";
				  	return;
				  }

				  if (pDriveEntry.DataModifications.empty()) {
				  	CATAPULT_LOG( warning ) << "received opinion but empty active modifications";
				  	return;
				  }

				  auto expectedSumBytes = m_storageState.getDownloadWorkBytes(opinion.m_replicatorKey, info.m_driveKey);

				  auto donatorShardExtended = m_storageState.getDonatorShardExtended(info.m_driveKey, opinion.m_replicatorKey);

				  uint64_t actualSumBytes = 0;

				  for (const auto& layout: opinion.m_uploadLayout) {
				  	uint64_t initialCumulativeUploadSize;
				  	if ( auto it = donatorShardExtended.m_actualShardMembers.find(layout.m_key); it != donatorShardExtended.m_actualShardMembers.end() ) {
				  		initialCumulativeUploadSize = it->second;
				  	}
				  	else if (auto it = donatorShardExtended.m_formerShardMembers.find(layout.m_key); it != donatorShardExtended.m_formerShardMembers.end()) {
				  		initialCumulativeUploadSize = it->second;
				  	}
				  	else if (layout.m_key == pDriveEntry.Owner.array()){
				  		initialCumulativeUploadSize = donatorShardExtended.m_ownerUpload;
				  	}
				  	else {
				  		CATAPULT_LOG( warning ) << "received modification opinion from incorrect replicator " << Key(layout.m_key);
				  		return;
				  	}

				  	if (layout.m_uploadedBytes < initialCumulativeUploadSize) {
				  		CATAPULT_LOG( warning ) << "received modification with negative increment";
				  		return;
				  	}

				  	auto actualSumBytesTemp = actualSumBytes + layout.m_uploadedBytes;

				  	if (actualSumBytesTemp < actualSumBytes) {
				  		CATAPULT_LOG( warning ) << "received modification with overflow increment";
						return;
					}

				  	actualSumBytes = actualSumBytesTemp;
				  }

				  auto modificationIt = std::find_if(
				  		pDriveEntry.DataModifications.begin(),
				  		pDriveEntry.DataModifications.end(),
				  		[&info](const auto& item) { return item.Id == info.m_modifyTransactionHash; }
				  		);

				  if (modificationIt == pDriveEntry.DataModifications.end()) {
				  	CATAPULT_LOG( warning ) << "received opinion for non-existing modification";
				  	return;
				  }

				  modificationIt++;
				  expectedSumBytes += std::accumulate(
				  		pDriveEntry.DataModifications.begin(),
				  		modificationIt,
				  		static_cast<uint64_t>(0),
				  		[](const auto& accumulator, const auto& currentModification) {
				  			return accumulator + utils::FileSize::FromMegabytes(currentModification.ActualUploadSize).bytes();
				  		}
				  		);

				  if (actualSumBytes != expectedSumBytes) {
				  	CATAPULT_LOG( warning ) << "received opinion with invalid opinion sum " << expectedSumBytes << " " << actualSumBytes;
				  	return;
				  }

				  pReplicator->asyncOnOpinionReceived(info);
				});
            }

            void downloadOpinionHasBeenReceived(
                    sirius::drive::Replicator&,
                    const sirius::drive::DownloadApprovalTransactionInfo& info) override {
            	boost::asio::post(m_pool->ioContext(), [this, info] {
					auto pReplicator = m_pReplicator.lock();
					if (!pReplicator)
						return;

					if (!m_storageState.downloadChannelExists(info.m_downloadChannelId)) {
						CATAPULT_LOG(error) << "download channel opinion received id does not exist "
											<< Hash256(info.m_downloadChannelId);
						return;
					}

					if (info.m_opinions.size() != 1) {
						CATAPULT_LOG(warning) << "received download opinion with many opinions";
						return;
					}

					bool sorted = true;

					const auto& uploadLayout = info.m_opinions[0].m_downloadLayout;
					for (int i = 0; i < uploadLayout.size() - 1 && sorted; i++) {
						if (Key(uploadLayout[i + 1].m_key) < Key(uploadLayout[i].m_key)) {
							sorted = false;
						}
					}

					if (!sorted) {
						CATAPULT_LOG( error ) << "received unsorted modification opinion";
						return;
					}

					const auto& opinion = info.m_opinions.at(0);
					if (!opinion.Verify(info.m_blockHash, info.m_downloadChannelId)) {
						CATAPULT_LOG(warning) << "received download opinion with incorrect signature";
						return;
					}

					const auto& downloadLayout = opinion.m_downloadLayout;
					if (std::find_if(downloadLayout.begin(), downloadLayout.end(), [&opinion](const auto& item) {
							return opinion.m_replicatorKey == opinion.m_downloadLayout.front().m_key;
						}) == downloadLayout.end()) {
						CATAPULT_LOG(warning) << "received download opinion, no opinion on itself";
					}

					if (!m_storageState.isReplicatorAssignedToChannel(
								opinion.m_replicatorKey, info.m_downloadChannelId)) {
						CATAPULT_LOG(warning) << "received download opinion from wrong replicator";
						return;
					}

					pReplicator->asyncOnDownloadOpinionReceived(info);
				});
			}

        private:
        	std::shared_ptr<thread::IoThreadPool> m_pool;
            TransactionSender m_transactionSender;
            state::StorageState& m_storageState;
            TransactionStatusHandler& m_transactionStatusHandler;
            const crypto::KeyPair& m_keyPair;
        };
    }

    std::unique_ptr<ReplicatorEventHandler> CreateReplicatorEventHandler(
    		std::shared_ptr<thread::IoThreadPool>&& pool,
            TransactionSender&& transactionSender,
            state::StorageState& storageState,
            TransactionStatusHandler& operations,
			const catapult::crypto::KeyPair& keyPair) {
    	return std::make_unique<DefaultReplicatorEventHandler>(
				std::move(pool), std::move(transactionSender), storageState, operations, keyPair);
    }
}}
