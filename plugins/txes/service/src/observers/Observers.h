/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/observers/ObserverTypes.h"
#include "src/config/ServiceConfiguration.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/ServiceReceiptType.h"
#include "src/state/DriveEntry.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by prepare drive notifications.
	DECLARE_OBSERVER(PrepareDriveV1, model::PrepareDriveNotification<1>)();

	/// Observes changes triggered by prepare drive notifications.
	DECLARE_OBSERVER(PrepareDriveV2, model::PrepareDriveNotification<2>)();

	/// Observes changes triggered by drive file system notifications.
	DECLARE_OBSERVER(DriveFileSystem, model::DriveFileSystemNotification<1>)(const MosaicId& streamingMosaicId);

	/// Observes changes triggered by files deposit notifications.
	DECLARE_OBSERVER(FilesDeposit, model::FilesDepositNotification<1>)();

	/// Observes changes triggered by drive verification payment notifications.
	DECLARE_OBSERVER(DriveVerificationPayment, model::DriveVerificationPaymentNotification<1>)(const MosaicId& storageMosaicId);

	/// Observes changes triggered by join to drive notifications.
	DECLARE_OBSERVER(JoinToDrive, model::JoinToDriveNotification<1>)();

	/// Observes changes triggered by exchange Xpx to SO units.

	DECLARE_OBSERVER(StartBilling, model::BalanceCreditNotification<1>)(const MosaicId& storageMosaicId);

	/// Observes changes triggered at the end of billing period.
	DECLARE_OBSERVER(EndBilling, model::BlockNotification<1>)(const MosaicId& storageMosaicId);

	/// Observes changes triggered by the en drive transaction.
	DECLARE_OBSERVER(EndDrive, model::EndDriveNotification<1>)(const config::ImmutableConfiguration& config);

	/// Observes changes triggered by drive files transaction.
	DECLARE_OBSERVER(DriveFilesReward, model::DriveFilesRewardNotification<1>)(const config::ImmutableConfiguration& config);

	/// Observes changes triggered by block.
	DECLARE_OBSERVER(DriveCacheBlockPruning, model::BlockNotification<1>)();

	/// Observes changes triggered by start file download.
	DECLARE_OBSERVER(StartFileDownload, model::StartFileDownloadNotification<1>)();

	/// Observes changes triggered by end file download.
	DECLARE_OBSERVER(EndFileDownload, model::EndFileDownloadNotification<1>)();

	/// Observes changes triggered by block.
	DECLARE_OBSERVER(ExpiredFileDownload, model::BlockNotification<1>)();
}}
