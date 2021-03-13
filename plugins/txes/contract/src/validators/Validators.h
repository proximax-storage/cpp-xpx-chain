/**
*** Copyright 2018 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/ContractNotifications.h"
#include "catapult/validators/StatefulValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to modify contract notifications and validates that:
	/// - same customer does not occur in removed and added
	DECLARE_STATELESS_VALIDATOR(ModifyContractCustomers, model::ModifyContractNotification<1>)();

	/// A validator implementation that applies to modify contract notifications and validates that:
	/// - same executor does not occur in removed and added
	DECLARE_STATELESS_VALIDATOR(ModifyContractExecutors, model::ModifyContractNotification<1>)();

	/// A validator implementation that applies to modify contract notifications and validates that:
	/// - same verifier does not occur in removed and added
	DECLARE_STATELESS_VALIDATOR(ModifyContractVerifiers, model::ModifyContractNotification<1>)();

	/// A validator implementation that applies to modify contract notifications and validates that:
	/// - added account isn't already a customer
	/// - removed account is already a customer
	DECLARE_STATEFUL_VALIDATOR(ModifyContractInvalidCustomers, model::ModifyContractNotification<1>)();

	/// A validator implementation that applies to modify contract notifications and validates that:
	/// - added account isn't already a executor
	/// - removed account is already a executor
	DECLARE_STATEFUL_VALIDATOR(ModifyContractInvalidExecutors, model::ModifyContractNotification<1>)();

	/// A validator implementation that applies to modify contract notifications and validates that:
	/// - added account isn't already a verifier
	/// - removed account is already a verifier
	DECLARE_STATEFUL_VALIDATOR(ModifyContractInvalidVerifiers, model::ModifyContractNotification<1>)();

	/// A validator implementation that applies to modify contract notifications and validates that:
	/// - duration is positive at contract creation
	DECLARE_STATEFUL_VALIDATOR(ModifyContractDuration, model::ModifyContractNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(ContractPluginConfig, model::PluginConfigNotification<1>)();
}}
