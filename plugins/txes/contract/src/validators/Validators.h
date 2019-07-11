/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#pragma once
#include "Results.h"
#include "src/model/ContractNotifications.h"
#include "catapult/validators/ValidatorContext.h"
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
	DECLARE_STATELESS_VALIDATOR(PluginConfig, model::PluginConfigNotification<1>)();
}}
