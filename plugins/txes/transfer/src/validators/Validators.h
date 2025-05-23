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
#include "src/model/TransferNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to transfer message notifications and validates that:
	/// - messages have a maximum message size of \a maxMessageSize
	DECLARE_STATEFUL_VALIDATOR(TransferMessage, model::TransferMessageNotification<1>)();

	/// A validator implementation that applies to transfer mosaics notifications and validates that:
	/// - mosaics are ordered, and mosaics have maximum count \a maxMosaicsSize, and mosaic amount is greater than zero.
	DECLARE_STATEFUL_VALIDATOR(TransferMosaics, model::TransferMosaicsNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(TransferPluginConfig, model::PluginConfigNotification<1>)();
}}
