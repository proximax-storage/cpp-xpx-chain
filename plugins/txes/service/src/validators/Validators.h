/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "src/model/ServiceNotifications.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"
#include "plugins/txes/exchange/src/model/ExchangeNotifications.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to prepare drive notifications and validates that:
	/// - drive duration is not zero
	/// - drive size is not zero
	/// - number of drive replicas is not zero
	/// - count of min replicators is not zero
	/// - duration % billingPeriod == 0
	/// - Percent approvers in range 0-100
	/// - Min replicators <= replicas
	DECLARE_STATELESS_VALIDATOR(PrepareDriveArguments, model::PrepareDriveNotification<1>)();

	/// A validator implementation that applies to drive prepare drive notifications and validates that:
	/// - the drive is not exist
	DECLARE_STATEFUL_VALIDATOR(PrepareDrivePermission, model::PrepareDriveNotification<1>)();

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
	DECLARE_STATEFUL_VALIDATOR(MaxFilesOnDrive, model::DriveFileSystemNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// A validator check that exchange from replicators are valid
	DECLARE_STATEFUL_VALIDATOR(Exchange, model::ExchangeNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// A validator check that end drive transaction is permitted
	DECLARE_STATEFUL_VALIDATOR(EndDrive, model::EndDriveNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// A validator check that:
	/// - Replicator is part of drive
	/// - Files without deposit
	DECLARE_STATEFUL_VALIDATOR(FilesDeposit, model::FilesDepositNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(ServicePluginConfig, model::PluginConfigNotification<1>)();
}}
