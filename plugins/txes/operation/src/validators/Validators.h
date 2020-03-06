/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/OperationNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to operation duration notifications and validates that:
	/// - operation duration is at most \a maxOperationDuration
	DECLARE_STATEFUL_VALIDATOR(OperationDuration, model::OperationDurationNotification<1>)();

	/// A validator implementation that applies to start operation notifications and validates that:
	/// - executors not duplicated
	DECLARE_STATELESS_VALIDATOR(StartOperation, model::StartOperationNotification<1>)();

	/// A validator implementation that applies to end operation notifications and validates that:
	/// - operation token is valid
	/// - operation is in-progress
	/// - operation executor is valid
	/// - spent mosaics are valid
	DECLARE_STATEFUL_VALIDATOR(EndOperation, model::EndOperationNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(OperationPluginConfig, model::PluginConfigNotification<1>)();

	/// A validator implementation that applies to start operation notifications and validates that:
	/// - mosaic ids are not duplicated
	/// - mosaic amount is not zero
	DECLARE_STATELESS_VALIDATOR(OperationMosaic, model::OperationMosaicNotification<1>)();

	/// A validator implementation that applies to aggregate cosignatures notifications and validates that:
	/// - operation identify transaction is the very first sub transaction
	/// - end operation transaction is the very last sub transaction
	/// - operation identify transaction is not aggregated with end operation transaction
	DECLARE_STATELESS_VALIDATOR(AggregateTransaction, model::AggregateCosignaturesNotification<1>)();

	/// A validator implementation that applies to aggregate cosignatures notifications and validates that:
	/// - operation identify transaction contains valid operation token
	DECLARE_STATEFUL_VALIDATOR(OperationIdentify, model::AggregateCosignaturesNotification<1>)();

	/// A validator implementation that applies to aggregate embedded transaction notifications and validates that:
	/// - operation identify and end operation transactions are the very first in aggregate
	DECLARE_STATELESS_VALIDATOR(EmbeddedTransaction, model::AggregateEmbeddedTransactionNotification<1>)();
}}
