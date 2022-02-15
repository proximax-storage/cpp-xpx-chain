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
#include "src/model/LockFundNotifications.h"
#include "catapult/config_holder/BlockchainConfigurationHolder.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	/// A validator implementation that applies to lock fund notifications and validates that:
	/// - mosaics are ordered, and mosaics have maximum count \a maxMosaicsSize, and mosaic amount is greater than zero.
	/// - there are enough mosaics to either lock/unlock
	/// - the signing account is of version 2 or higher and exists in cache
	/// - there are no existing unlock requests at the requested unlock height for this signer
	DECLARE_STATEFUL_VALIDATOR(LockFundTransfer, model::LockFundTransferNotification<1>)();

	/// A validator implementation that applies to lock fund cancel unlock notifications and validates that:
	/// - a record exists at the specified height for the specified key.
	DECLARE_STATEFUL_VALIDATOR(LockFundCancelUnlock, model::LockFundCancelUnlockNotification<1>)();

	/// A validator implementation that applies to plugin config notification and validates that:
	/// - plugin configuration is valid
	DECLARE_STATELESS_VALIDATOR(LockFundPluginConfig, model::PluginConfigNotification<1>)();
}}
