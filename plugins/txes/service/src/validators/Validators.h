/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/StatefulValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "src/model/ServiceNotifications.h"
#include "src/state/DriveEntry.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"
#include "plugins/txes/exchange/src/model/ExchangeNotifications.h"

namespace catapult { namespace validators {

	void VerificationStatus(const state::DriveEntry& driveEntry, const validators::StatefulValidatorContext& context, bool& started, bool& active);

	/// A validator implementation that applies to prepare drive notifications and validates that:
	/// - drive duration is not zero
	/// - drive size is not zero
	/// - number of drive replicas is not zero
	/// - count of min replicators is not zero
	/// - duration % billingPeriod == 0
	/// - Percent approvers in range 0-100
	/// - Min replicators <= replicas
	DECLARE_STATELESS_VALIDATOR(PrepareDriveArgumentsV1, model::PrepareDriveNotification<1>)();
	DECLARE_STATELESS_VALIDATOR(PrepareDriveArgumentsV2, model::PrepareDriveNotification<2>)();

	/// A validator implementation that applies to drive prepare drive notifications and validates that:
	/// - the drive is not exist
	DECLARE_STATEFUL_VALIDATOR(PrepareDrivePermissionV1, model::PrepareDriveNotification<1>)();
	DECLARE_STATEFUL_VALIDATOR(PrepareDrivePermissionV2, model::PrepareDriveNotification<2>)();

	/// A validator check that operation is permitted by drive multisig.
	DECLARE_STATEFUL_VALIDATOR(DrivePermittedOperation, model::AggregateEmbeddedTransactionNotification<1>)();

	/// A validator check that:
	/// - Drive is exists
	/// - Replicator is not participant in drive
	DECLARE_STATEFUL_VALIDATOR(JoinToDrive, model::JoinToDriveNotification<1>)();

	/// A validator check that:
	/// - Drive is exists
	/// - Signer is owner of drive
	/// - Root hash in transaction the same like in db
	/// - Added files are new and removed files are exist
	DECLARE_STATEFUL_VALIDATOR(DriveFileSystem, model::DriveFileSystemNotification<1>)();

	/// A validator check that drive exist and it is not finished
	DECLARE_STATEFUL_VALIDATOR(Drive, model::DriveNotification<1>)();

	/// A validator check that drive contains not many files
	DECLARE_STATEFUL_VALIDATOR(MaxFilesOnDrive, model::DriveFileSystemNotification<1>)();

	/// A validator check that exchange from replicators are valid
	DECLARE_STATEFUL_VALIDATOR(ExchangeV1, model::ExchangeNotification<1>)();
	DECLARE_STATEFUL_VALIDATOR(ExchangeV2, model::ExchangeNotification<2>)();

	/// A validator check that end drive transaction is permitted
	DECLARE_STATEFUL_VALIDATOR(EndDrive, model::EndDriveNotification<1>)();

	/// A validator check that replicators are valid(they are part of drive and put deposit for active files). That drive contains streaming tokens and drive in pending or finished state
	DECLARE_STATEFUL_VALIDATOR(DriveFilesReward, model::DriveFilesRewardNotification<1>)(const MosaicId& streamingMosaicId);

	/// A validator check that:
	/// - Replicator is part of drive
	/// - Files without deposit
	DECLARE_STATEFUL_VALIDATOR(FilesDeposit, model::FilesDepositNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(ServicePluginConfig, model::PluginConfigNotification<1>)();

	/// A validator check that:
	/// - Drive is in correct state
	/// - Initiator is registered to drive
	/// - Another verification is not in progress
	DECLARE_STATEFUL_VALIDATOR(StartDriveVerification, model::StartDriveVerificationNotification<1>)();

	/// A validator check that:
	/// - Verification is in progress
	/// - Failed replicators are registered to drive
	DECLARE_STATEFUL_VALIDATOR(EndDriveVerification, model::EndDriveVerificationNotification<1>)();

	/// A validator implementation that applies to failed block hashes notification and validates that:
	/// - there is at least one block hash
	/// - there are no duplicate hashes
	DECLARE_STATELESS_VALIDATOR(FailedBlockHashes, model::FailedBlockHashesNotification<1>)();

	/// A validator check that:
	/// - Drive is in correct state
	/// - File recipient is not drive replicator
	/// - There is at least one file to download
	/// - Files exist
	/// - File sizes are valid
	/// - File hashes are not duplicated
	/// - File downloading is not in progress
	DECLARE_STATEFUL_VALIDATOR(StartFileDownload, model::StartFileDownloadNotification<1>)();

	/// - File downloading is in progress
	DECLARE_STATEFUL_VALIDATOR(EndFileDownload, model::EndFileDownloadNotification<1>)();
}}
