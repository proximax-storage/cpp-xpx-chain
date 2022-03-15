/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DataModificationApprovalTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/DataModificationApprovalTransaction.h"
#include "src/utils/StorageUtils.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));

				const auto commonDataSize = sizeof(transaction.DriveKey)
											+ sizeof(transaction.DataModificationId)
											+ sizeof(transaction.FileStructureCdi)
											+ sizeof(transaction.FileStructureSizeBytes)
											+ sizeof(transaction.MetaFilesSizeBytes)
											+ sizeof(transaction.UsedDriveSizeBytes);
				auto* const pCommonDataBegin = sub.mempool().malloc<uint8_t>(commonDataSize);
				auto* pCommonData = pCommonDataBegin;
				utils::WriteToByteArray(pCommonData, transaction.DriveKey);
				utils::WriteToByteArray(pCommonData, transaction.DataModificationId);
				utils::WriteToByteArray(pCommonData, transaction.FileStructureCdi);
				utils::WriteToByteArray(pCommonData, transaction.FileStructureSizeBytes);
				utils::WriteToByteArray(pCommonData, transaction.MetaFilesSizeBytes);
				utils::WriteToByteArray(pCommonData, transaction.UsedDriveSizeBytes);

				sub.notify(OpinionNotification<1>(
						commonDataSize,
						transaction.JudgingKeysCount,
						transaction.OverlappingKeysCount,
						transaction.JudgedKeysCount,
						sizeof(uint64_t),
						pCommonDataBegin,
						transaction.PublicKeysPtr(),
						transaction.SignaturesPtr(),
						transaction.PresentOpinionsPtr(),
						reinterpret_cast<const uint8_t*>(transaction.OpinionsPtr())
				));

				// Must be applied before UsedSize of the drive is changed,
				// i.e. before the DataModificationApproval notification.
				sub.notify(DataModificationApprovalRefundNotification<1>(
						transaction.DriveKey,
						transaction.DataModificationId,
						transaction.UsedDriveSizeBytes,
						transaction.MetaFilesSizeBytes));

				sub.notify(DataModificationApprovalNotification<1>(
						transaction.DriveKey,
						transaction.DataModificationId,
						transaction.FileStructureCdi,
						transaction.FileStructureSizeBytes,
						transaction.MetaFilesSizeBytes,
						transaction.UsedDriveSizeBytes,
						transaction.JudgingKeysCount,
						transaction.OverlappingKeysCount,
						transaction.JudgedKeysCount,
						transaction.PublicKeysPtr(),
						transaction.PresentOpinionsPtr()
				));

				// Makes mosaic transfers;
				// Updates cosigner replicators' drive infos (updates LastApprovedDataModificationId,
				// resets InitialDownloadWork, calculates LastCompletedCumulativeDownloadWork)
				sub.notify(DataModificationApprovalDownloadWorkNotification<1>(
						transaction.DriveKey,
						transaction.DataModificationId,
						transaction.JudgingKeysCount + transaction.OverlappingKeysCount,
						transaction.PublicKeysPtr()
				));

				// Makes mosaic transfers;
				// Updates drive's replicator infos (updates CumulativeUploadPayments) and owner cumulative upload size.
				sub.notify(DataModificationApprovalUploadWorkNotification<1>(
						transaction.DriveKey,
						transaction.DataModificationId,
						transaction.JudgingKeysCount,
						transaction.OverlappingKeysCount,
						transaction.JudgedKeysCount,
						transaction.PublicKeysPtr(),
						transaction.PresentOpinionsPtr(),
						transaction.OpinionsPtr()
				));

				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of DataModificationApprovalTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(DataModificationApproval, Default, Publish)
}}
