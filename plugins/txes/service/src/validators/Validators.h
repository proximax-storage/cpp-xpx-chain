/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/ServiceNotifications.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to transfer mosaics notification and validates that:
	/// - number of mosaics is zero or one
	/// - mosaic amount is not zero
	DECLARE_STATELESS_VALIDATOR(TransferMosaics, model::TransferMosaicsNotification<1>)();

	/// A validator implementation that applies to drive notifications and validates that:
	/// - the drive exists
	DECLARE_STATEFUL_VALIDATOR(Drive, model::DriveNotification<1>)();

	/// A validator implementation that applies to replicator notifications and validates that:
	/// - the drive replicator is registered
	DECLARE_STATEFUL_VALIDATOR(Replicator, model::ReplicatorNotification<1>)();

	/// A validator implementation that applies to mosaic id notification and validates that:
	/// - the mosaic ids are equal
	DECLARE_STATELESS_VALIDATOR(MosaicId, model::MosaicIdNotification<1>)();

	/// A validator implementation that applies to prepare drive notifications and validates that:
	/// - drive key is multisig
	/// - drive duration is not zero
	/// - drive size is not zero
	/// - number of drive replicas is not zero
	DECLARE_STATEFUL_VALIDATOR(PrepareDrive, model::PrepareDriveNotification<1>)();

	/// A validator implementation that applies to drive prolongation notifications and validates that:
	/// - the drive exists
	/// - drive duration prolongation is not zero
	DECLARE_STATELESS_VALIDATOR(DriveProlongation, model::DriveProlongationNotification<1>)();

	/// A validator implementation that applies to drive deposit notifications and validates that:
	/// - the drive deposit is big enough
	DECLARE_STATEFUL_VALIDATOR(DriveDeposit, model::DriveDepositNotification<1>)();

	/// A validator implementation that applies to drive deposit return notifications and validates that:
	/// - the drive deposit is not yet returned
	/// - the returned deposit is equal the deposit made by the replicator
	DECLARE_STATEFUL_VALIDATOR(DriveDepositReturn, model::DriveDepositReturnNotification<1>)();

	/// A validator implementation that applies to drive deposit return notifications and validates that:
	/// - the file deposit is not yet returned
	/// - the returned file deposit is equal the deposit made by the replicator
	DECLARE_STATEFUL_VALIDATOR(FileDepositReturn, model::FileDepositReturnNotification<1>)();

	/// A validator implementation that applies to create drive directory notifications and validates that:
	/// - the parent drive directory exists
	/// - the drive directory doesn't exist
	DECLARE_STATEFUL_VALIDATOR(CreateDirectory, model::CreateDirectoryNotification<1>)();

	/// A validator implementation that applies to remove drive directory notifications and validates that:
	/// - the drive directory exists
	DECLARE_STATEFUL_VALIDATOR(RemoveDirectory, model::RemoveDirectoryNotification<1>)();

	/// A validator implementation that applies to upload drive file notifications and validates that:
	/// - the parent drive directory exists
	/// - the drive file doesn't exist
	DECLARE_STATEFUL_VALIDATOR(UploadFile, model::UploadFileNotification<1>)();

	/// A validator implementation that applies to download drive file notifications and validates that:
	/// - the drive file exists
	DECLARE_STATEFUL_VALIDATOR(DownloadFile, model::DownloadFileNotification<1>)();

	/// A validator implementation that applies to delete drive file notifications and validates that:
	/// - the drive file exists
	DECLARE_STATEFUL_VALIDATOR(DeleteFile, model::DeleteFileNotification<1>)();

	/// A validator implementation that applies to move drive file notifications and validates that:
	/// - the destination file and source file are from the same drive
	/// - the destination file and source file have the same hash
	/// - the parent directory of the destination drive file exists
	/// - the source drive file exists
	DECLARE_STATEFUL_VALIDATOR(MoveFile, model::MoveFileNotification<1>)();

	/// A validator implementation that applies to copy drive file notifications and validates that:
	/// - the destination file and source file are from the same drive
	/// - the parent directory of the destination drive file exists
	/// - the source drive file exists
	/// - the destination drive file doesn't exist
	DECLARE_STATEFUL_VALIDATOR(CopyFile, model::CopyFileNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(ServicePluginConfig, model::PluginConfigNotification<1>)();
}}
