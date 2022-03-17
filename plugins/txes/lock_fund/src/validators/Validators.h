/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
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
