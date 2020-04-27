/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "src/model/SuperContractNotifications.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"
#include "plugins/txes/service/src/model/ServiceNotifications.h"

namespace catapult { namespace validators {

	/// A validator check that deploy transaction is valid
	DECLARE_STATEFUL_VALIDATOR(Deploy, model::DeployNotification<1>)();

	/// A validator implementation that applies to start execute notification and validates that:
	/// - execution count doesn't overflow
	DECLARE_STATEFUL_VALIDATOR(StartExecute, model::StartExecuteNotification<1>)();

	/// A validator implementation that applies to end execute notification and validates that:
	/// - execution count is not zero
	DECLARE_STATEFUL_VALIDATOR(EndExecute, model::EndExecuteNotification<1>)();

	/// TODO: During implementation of deactivating super contract also remember about DriveFinishNotification
	/// A validator check that drive file system transaction doesn't remove super contract file
	DECLARE_STATEFUL_VALIDATOR(DriveFileSystem, model::DriveFileSystemNotification<1>)();

	/// A validator check that super contract is exist
	DECLARE_STATEFUL_VALIDATOR(SuperContract, model::SuperContractNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(SuperContractPluginConfig, model::PluginConfigNotification<1>)();

	/// A validator implementation that applies to aggregate cosignatures notifications and validates that:
	/// - operation identify transaction is the very first sub transaction
	/// - end execute transaction is the very last sub transaction
	/// - operation identify transaction is not aggregated with end execute transaction
	DECLARE_STATELESS_VALIDATOR(AggregateTransaction, model::AggregateCosignaturesNotification<1>)();

	/// A validator implementation that applies to aggregate embedded transaction notifications and validates that:
	/// - end operation transaction doesn't end super contract execution
	DECLARE_STATEFUL_VALIDATOR(EndOperationTransaction, model::AggregateEmbeddedTransactionNotification<1>)();

	/// A validator check that deactivate transaction is valid
	DECLARE_STATEFUL_VALIDATOR(Deactivate, model::DeactivateNotification<1>)();

	/// A validator check that suspend transaction is valid
	DECLARE_STATEFUL_VALIDATOR(Suspend, model::SuspendNotification<1>)();

	/// A validator check that resume transaction is valid
	DECLARE_STATEFUL_VALIDATOR(Resume, model::ResumeNotification<1>)();
}}
