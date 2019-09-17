/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
//#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/observers/ObserverTypes.h"
#include "src/config/ServiceConfiguration.h"
#include "src/model/ServiceNotifications.h"

namespace catapult { namespace observers {

	/// Observes changes triggered by prepare drive notifications.
	DECLARE_OBSERVER(PrepareDrive, model::PrepareDriveNotification<1>)();

	/// Observes changes triggered by drive prolongation notifications.
	DECLARE_OBSERVER(DriveProlongation, model::DriveProlongationNotification<1>)();

	/// Observes changes triggered by drive deposit notifications.
	DECLARE_OBSERVER(DriveDeposit, model::DriveDepositNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// Observes changes triggered by drive deposit notifications.
	DECLARE_OBSERVER(DriveDepositReturn, model::DriveDepositReturnNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// Observes changes triggered by drive file deposit notifications.
	DECLARE_OBSERVER(FileDeposit, model::FileDepositNotification<1>)();

	/// Observes changes triggered by drive verification notifications.
	DECLARE_OBSERVER(DriveVerification, model::DriveVerificationNotification<1>)();

	/// Observes changes triggered by create drive directory notifications.
	DECLARE_OBSERVER(CreateDirectory, model::CreateDirectoryNotification<1>)();

	/// Observes changes triggered by remove drive directory notifications.
	DECLARE_OBSERVER(RemoveDirectory, model::RemoveDirectoryNotification<1>)();

	/// Observes changes triggered by upload drive file notifications.
	DECLARE_OBSERVER(UploadFile, model::UploadFileNotification<1>)();

	/// Observes changes triggered by delete drive file notifications.
	DECLARE_OBSERVER(DeleteFile, model::DeleteFileNotification<1>)();

	/// Observes changes triggered by move drive file notifications.
	DECLARE_OBSERVER(MoveFile, model::MoveFileNotification<1>)();

	/// Observes changes triggered by move drive file notifications.
	DECLARE_OBSERVER(CopyFile, model::CopyFileNotification<1>)();
}}
