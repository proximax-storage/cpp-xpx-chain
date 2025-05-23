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
#include "src/model/MultisigNotifications.h"
#include "plugins/txes/aggregate/src/model/AggregateNotifications.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to modify multisig cosigners notifications and validates that:
	/// - same account does not occur in removed and added cosignatories
	/// - there is at most one cosignatory removed
	stateless::NotificationValidatorPointerT<model::ModifyMultisigCosignersNotification<1>>
	CreateModifyMultisigCosignersValidator();

	/// A validator implementation that applies to transaction notifications and validates that:
	/// - multisig accounts cannot initiate transactions
	stateful::NotificationValidatorPointerT<model::TransactionNotification<1>>
	CreateMultisigPermittedOperationValidator();

	/// A validator implementation that applies to modify multisig cosigners notifications and validates that:
	/// - added account isn't already a cosignatory
	/// - removed account is already a cosignatory
	stateful::NotificationValidatorPointerT<model::ModifyMultisigCosignersNotification<1>>
	CreateModifyMultisigInvalidCosignersValidator();

	/// A validator implementation that applies to modify multisig new cosigner notifications and validates that:
	/// - the cosignatory is cosigning at most \a maxCosignedAccountsPerAccount
	stateful::NotificationValidatorPointerT<model::ModifyMultisigNewCosignerNotification<1>>
	CreateModifyMultisigMaxCosignedAccountsValidator();

	/// A validator implementation that applies to modify multisig cosigners notifications and validates that:
	/// - the multisig account has at most \a maxCosignersPerAccount cosignatories
	stateful::NotificationValidatorPointerT<model::ModifyMultisigCosignersNotification<1>>
	CreateModifyMultisigMaxCosignersValidator();

	/// A validator implementation that applies to modify multisig new cosigner notifications and validates that:
	/// - the multisig depth is at most \a maxMultisigDepth
	/// - no multisig loops are created
	stateful::NotificationValidatorPointerT<model::ModifyMultisigNewCosignerNotification<1>>
	CreateModifyMultisigLoopAndLevelValidator();

	/// A validator implementation that applies to modify multisig settings notifications and validates that:
	/// - new min removal and min approval are greater than 0
	/// - new min removal and min approval settings are not greater than total number of cosignatories
	stateful::NotificationValidatorPointerT<model::ModifyMultisigSettingsNotification<1>>
	CreateModifyMultisigInvalidSettingsValidator();

	/// A validator implementation that applies to aggregate cosignatures notifications and validates that:
	///  - all cosigners are eligible counterparties (using \a transactionRegistry to retrieve custom approval requirements)
	DECLARE_STATEFUL_VALIDATOR(
			MultisigAggregateEligibleCosigners,
			model::AggregateCosignaturesNotification<1>)(const model::TransactionRegistry& transactionRegistry);

	/// Validator that applies to aggregate embeded transaction notifications and validates that:
	///  - present cosigners are sufficient (using \a transactionRegistry to retrieve custom approval requirements)
	DECLARE_STATEFUL_VALIDATOR(
			MultisigAggregateSufficientCosigners,
			model::AggregateEmbeddedTransactionNotification<1>)(const model::TransactionRegistry& transactionRegistry);

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(MultisigPluginConfig, model::PluginConfigNotification<1>)();
}}
