/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorService.h"
#include "TransactionSender.h"
#include "TransactionStatusHandler.h"
#include "plugins/txes/storage/src/validators/Results.h"
#include <numeric>
#include <iostream>

namespace catapult { namespace storage {

	void ReplicatorService::modifyApprovalTransactionIsReady(
			sirius::drive::Replicator&,
			const sirius::drive::ApprovalTransactionInfo& info) {
		post([info](const auto& pThis) {
			CATAPULT_LOG(debug) << "sending data modification approval transaction";
			auto transactionHash = pThis->m_pTransactionSender->sendDataModificationApprovalTransaction(info);
			auto pTransactionStatusHandler = pThis->transactionStatusHandler();
			if (!pTransactionStatusHandler)
				return;

			auto pThisWeak = std::weak_ptr<ReplicatorService>(pThis);
			pTransactionStatusHandler->addHandler(transactionHash, [transactionHash, driveKey = info.m_driveKey, pThisWeak](uint32_t status) {
				auto pThis = pThisWeak.lock();
				if (!pThis)
					return;

				auto pReplicator = pThis->replicator();
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
		});
	}

	void ReplicatorService::singleModifyApprovalTransactionIsReady(
			sirius::drive::Replicator&,
			const sirius::drive::ApprovalTransactionInfo& info) {
		CATAPULT_LOG(debug) << "sending data modification single approval transaction";
		m_pTransactionSender->sendDataModificationSingleApprovalTransaction(info);
	}

	void ReplicatorService::downloadApprovalTransactionIsReady(
			sirius::drive::Replicator&,
			const sirius::drive::DownloadApprovalTransactionInfo& info) {
		post([info](const auto& pThis) {
			CATAPULT_LOG(debug) << "sending download approval transaction";
			auto transactionHash = pThis->m_pTransactionSender->sendDownloadApprovalTransaction(info);

			auto pTransactionStatusHandler = pThis->transactionStatusHandler();
			if (!pTransactionStatusHandler)
				return;

			auto pThisWeak = std::weak_ptr<ReplicatorService>(pThis);
			pTransactionStatusHandler->addHandler(transactionHash, [blockHash = info.m_blockHash, channelId = info.m_downloadChannelId, pThisWeak](uint32_t status) {
				auto pThis = pThisWeak.lock();
				if (!pThis)
					return;

				auto pReplicator = pThis->replicator();
				if (!pReplicator)
					return;

				auto validationResult = validators::ValidationResult(status);
				CATAPULT_LOG(debug) << "download approval transaction completed with " << validationResult;

				std::set<validators::ValidationResult> handledResults = {
						validators::Failure_Storage_Signature_Count_Insufficient,
						validators::Failure_Storage_Opinion_Invalid_Key
				};

				if (handledResults.find(validationResult) != handledResults.end())
					pReplicator->asyncDownloadApprovalTransactionHasFailedInvalidOpinions(blockHash, channelId);
			});
		});
	}

	void ReplicatorService::verificationTransactionIsReady(
			sirius::drive::Replicator&,
			const sirius::drive::VerifyApprovalTxInfo& info) {
		CATAPULT_LOG(debug) << "sending end drive verification transaction";
		m_pTransactionSender->sendEndDriveVerificationTransaction(info);
	}

	void ReplicatorService::opinionHasBeenReceived(
			sirius::drive::Replicator&,
			const sirius::drive::ApprovalTransactionInfo& info) {
		post([info](const auto& pThis) {
			CATAPULT_LOG(debug) << "modificationOpinionHasBeenReceived() " << int(info.m_opinions[0].m_replicatorKey[0]);
			auto pReplicator = pThis->replicator();
			if (!pReplicator)
				return;

			auto pDrive = pThis->getDrive(info.m_driveKey);
			if (!pDrive) {
				CATAPULT_LOG(warning) << "received modification opinion for a non existing drive";
				return;
			}

			if (info.m_opinions.size() != 1) {
				CATAPULT_LOG(warning) << "received modification opinion with many opinions";
				return;
			}

			const auto& uploadLayout = info.m_opinions[0].m_uploadLayout;
			if (!std::is_sorted(uploadLayout.begin(), uploadLayout.end(), [](const auto& lhs, const auto& rhs) { return Key(lhs.m_key) < Key(rhs.m_key); })) {
				CATAPULT_LOG(error) << "received unsorted modification opinion";
				return;
			}

			const auto& opinion = info.m_opinions.at(0);
			auto isValid = opinion.Verify(
				reinterpret_cast<const sirius::crypto::KeyPair&>(pThis->m_keyPair),
				info.m_driveKey,
				info.m_modifyTransactionHash,
				info.m_rootHash,
				info.m_status,
				info.m_fsTreeFileSize,
				info.m_metaFilesSize,
				info.m_driveSize);

			if (!isValid) {
				CATAPULT_LOG(warning) << "received opinion with incorrect signature";
				return;
			}

			const auto& replicators = pDrive->Replicators;
			auto it = replicators.find(opinion.m_replicatorKey);
			if (it == replicators.end()) {
				CATAPULT_LOG(warning) << "received opinion from not a replicator";
				return;
			}

			if (pDrive->DataModifications.empty()) {
				CATAPULT_LOG(warning) << "received opinion but empty active modifications";
				return;
			}

			auto expectedSumBytes = pDrive->ReplicatorInfo.at(opinion.m_replicatorKey).LastCompletedCumulativeDownloadWorkBytes;
			auto donatorShard = pDrive->ModificationShards.at(opinion.m_replicatorKey);

			uint64_t actualSumBytes = 0;

			for (const auto& layout: opinion.m_uploadLayout) {
				uint64_t initialCumulativeUploadSize;
				if (auto it = donatorShard.ActualShardMembers.find(layout.m_key); it != donatorShard.ActualShardMembers.end()) {
					initialCumulativeUploadSize = it->second;
				} else if (auto it = donatorShard.FormerShardMembers.find(layout.m_key); it != donatorShard.FormerShardMembers.end()) {
					initialCumulativeUploadSize = it->second;
				} else if (layout.m_key == pDrive->Owner.array()){
					initialCumulativeUploadSize = donatorShard.OwnerUpload;
				} else {
					CATAPULT_LOG(warning) << "received modification opinion from incorrect replicator " << Key(layout.m_key);
					return;
				}

				if (layout.m_uploadedBytes < initialCumulativeUploadSize) {
					CATAPULT_LOG(warning) << "received modification with negative increment";
					return;
				}

				auto actualSumBytesTemp = actualSumBytes + layout.m_uploadedBytes;
				if (actualSumBytesTemp < actualSumBytes) {
					CATAPULT_LOG(warning) << "received modification with overflow increment " << actualSumBytes << " " << layout.m_uploadedBytes;
					return;
				}

				actualSumBytes = actualSumBytesTemp;
			}

			auto modificationIt = std::find_if(
				pDrive->DataModifications.begin(),
				pDrive->DataModifications.end(),
				[&info](const auto& item) {
					return item.Id == info.m_modifyTransactionHash;
				});

			if (modificationIt == pDrive->DataModifications.end()) {
				CATAPULT_LOG(warning) << "received opinion for non-existing modification";
				return;
			}

			modificationIt++;
			expectedSumBytes += std::accumulate(
				pDrive->DataModifications.begin(),
				modificationIt,
				static_cast<uint64_t>(0),
				[](const auto& accumulator, const auto& currentModification) {
					return accumulator + utils::FileSize::FromMegabytes(currentModification.ActualUploadSize).bytes();
				});

			if (actualSumBytes != expectedSumBytes) {
				CATAPULT_LOG(warning) << "received opinion with invalid opinion sum " << expectedSumBytes << " " << actualSumBytes;
				return;
			}

			pReplicator->asyncOnOpinionReceived(info);
		});
	}

	void ReplicatorService::downloadOpinionHasBeenReceived(
			sirius::drive::Replicator&,
			const sirius::drive::DownloadApprovalTransactionInfo& info) {
		post([info](const auto& pThis) {
			auto pReplicator = pThis->replicator();
			if (!pReplicator)
				return;

			auto pChannel = pThis->getDownloadChannel(info.m_downloadChannelId);
			if (!pChannel) {
				CATAPULT_LOG(error) << "download channel not found " << Hash256(info.m_downloadChannelId);
				return;
			}

			if (info.m_opinions.size() != 1) {
				CATAPULT_LOG(warning) << "received multiple download opinions";
				return;
			}

			const auto& uploadLayout = info.m_opinions[0].m_downloadLayout;
			if (!std::is_sorted(uploadLayout.begin(), uploadLayout.end(), [](const auto& lhs, const auto& rhs) { return Key(lhs.m_key) < Key(rhs.m_key); })) {
				CATAPULT_LOG(error) << "received unsorted modification opinion";
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

			if (pChannel->Replicators.find(opinion.m_replicatorKey) == pChannel->Replicators.cend()) {
				CATAPULT_LOG(warning) << "received download opinion from wrong replicator";
				return;
			}

			pReplicator->asyncOnDownloadOpinionReceived(std::make_unique<sirius::drive::DownloadApprovalTransactionInfo>(info));
		});
	}

	void ReplicatorService::onLibtorrentSessionError(const std::string& message) {
		std::string error = "Libtorrent Session Error: " + message;
		CATAPULT_THROW_RUNTIME_ERROR(error.c_str());
	}
}}
