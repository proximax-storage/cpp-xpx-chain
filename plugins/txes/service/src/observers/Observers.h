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

	void SetDriveState(state::DriveEntry& entry, ObserverContext& context, state::DriveState driveState);

	/// Observes changes triggered by prepare drive notifications.
	DECLARE_OBSERVER(PrepareDrive, model::PrepareDriveNotification<1>)();

	/// Observes changes triggered by drive file system notifications.
	DECLARE_OBSERVER(DriveFileSystem, model::DriveFileSystemNotification<1>)();

	/// Observes changes triggered by files deposit notifications.
	DECLARE_OBSERVER(FilesDeposit, model::FilesDepositNotification<1>)();

	/// Observes changes triggered by drive verification notifications.
	DECLARE_OBSERVER(DriveVerification, model::DriveVerificationNotification<1>)();

	/// Observes changes triggered by join to drive notifications.
	DECLARE_OBSERVER(JoinToDrive, model::JoinToDriveNotification<1>)();

	/// Observes changes triggered by exchange Xpx to SO units.
	DECLARE_OBSERVER(StartBilling, model::BalanceCreditNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// Observes changes triggered at the end of billing period.
	DECLARE_OBSERVER(EndBilling, model::BlockNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// Observes changes triggered by the en drive transaction.
	DECLARE_OBSERVER(EndDrive, model::EndDriveNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
}}
