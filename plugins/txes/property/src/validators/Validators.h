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
#include "src/model/PropertyNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

#define DECLARE_SHARED_VALIDATORS(VALUE_NAME) \
	/* A validator implementation that applies to unresolved property value property modification notifications and validates that: */ \
	/* - all property modification types are valid */ \
	DECLARE_STATELESS_VALIDATOR(VALUE_NAME##PropertyModificationTypes, model::Modify##VALUE_NAME##PropertyNotification_v1)(); \
	\
	/* A validator implementation that applies to unresolved property value property modification notifications and validates that: */ \
	/* - there is no redundant property modification */ \
	DECLARE_STATEFUL_VALIDATOR(VALUE_NAME##PropertyRedundantModification, model::Modify##VALUE_NAME##PropertyNotification_v1)(); \
	\
	/* A validator implementation that applies to resolved property value modification notifications and validates that: */ \
	/* - add modification does not add a known value */ \
	/* - delete modification does not delete an unknown value */ \
	DECLARE_STATEFUL_VALIDATOR(VALUE_NAME##PropertyValueModification, model::Modify##VALUE_NAME##PropertyValueNotification_v1)(); \
	\
	/* A validator implementation that applies to property notifications and validates that: */ \
	/* - the maximum number of modifications (\a maxPropertyValues) is not exceeded */ \
	/* - the maximum number of property values (\a maxPropertyValues) is not exeeded */ \
	DECLARE_STATEFUL_VALIDATOR(Max##VALUE_NAME##PropertyValues, model::Modify##VALUE_NAME##PropertyNotification_v1)();

	DECLARE_SHARED_VALIDATORS(Address)
	DECLARE_SHARED_VALIDATORS(Mosaic)
	DECLARE_SHARED_VALIDATORS(TransactionType)

	/// A validator implementation that applies to property type notifications and validates that:
	/// - property type is known
	DECLARE_STATELESS_VALIDATOR(PropertyType, model::PropertyTypeNotification<1>)();

	/// A validator implementation that applies to address property value property modification notifications and validates that:
	/// - property modification value for network with id \a networkIdentifier is valid
	DECLARE_STATELESS_VALIDATOR(PropertyAddressNoSelfModification, model::ModifyAddressPropertyValueNotification_v1)(
			model::NetworkIdentifier networkIdentifier);

	/// A validator implementation that applies to address interaction notifications and validates that:
	/// - the source address is allowed to interact with all participant addresses
	DECLARE_STATEFUL_VALIDATOR(AddressInteraction, model::AddressInteractionNotification<1>)();

	/// A validator implementation that applies to balance transfer notifications and validates that:
	/// - the mosaic is allowed to be transferred to the recipient
	DECLARE_STATEFUL_VALIDATOR(MosaicRecipient, model::BalanceTransferNotification<1>)();

	/// A validator implementation that applies to transaction type property modification notifications and validates that:
	/// - all transaction type property modification values are valid
	DECLARE_STATELESS_VALIDATOR(TransactionTypePropertyModificationValues, model::ModifyTransactionTypePropertyNotification_v1)();

	/// A validator implementation that applies to transaction notifications and validates that:
	/// - the signer is allowed to initiate a transaction of the specified transaction type
	DECLARE_STATEFUL_VALIDATOR(TransactionType, model::TransactionNotification<1>)();

	/// A validator implementation that applies to transaction type property value property modification notifications and validates that:
	/// - transaction type property transactions are not blocked
	DECLARE_STATEFUL_VALIDATOR(TransactionTypeNoSelfBlocking, model::ModifyTransactionTypePropertyValueNotification_v1)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(PropertyPluginConfig, model::PluginConfigNotification<1>)();
}}
