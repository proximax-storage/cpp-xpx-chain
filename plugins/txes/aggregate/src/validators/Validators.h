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
#include "src/model/AggregateNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to aggregate cosignatures notifications and validates that:
	/// - the number of transactions does not exceed \a maxTransactions
	/// - the number of implicit and explicit cosignatures does not exceed \a maxCosignatures
	/// - there are no redundant cosigners
	DECLARE_STATEFUL_VALIDATOR(BasicAggregateCosignatures, model::AggregateCosignaturesNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// A validator implementation that applies to aggregate cosignatures notifications and validates that:
	/// - the set of component signers is equal to the set of cosigners
	DECLARE_STATEFUL_VALIDATOR(StrictAggregateCosignatures, model::AggregateCosignaturesNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(AggregatePluginConfig, model::PluginConfigNotification<1>)();

	/// A validator implementation that applies to aggregate transaction entity types notifications and validates that:
	/// - aggregate bonded transaction is enabled
	DECLARE_STATEFUL_VALIDATOR(AggregateTransactionType, model::AggregateTransactionTypeNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder);
}}
