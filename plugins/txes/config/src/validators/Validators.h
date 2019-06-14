/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/CatapultConfigNotifications.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to catapult config signer notification and validates that:
	/// - signer is nemesis account
	DECLARE_STATEFUL_VALIDATOR(CatapultConfigSigner, model::CatapultConfigSignerNotification<1>)();

	/// A validator implementation that applies to catapult config notification and validates that:
	/// - blockchain configuration data size does not exceed the limit
	/// - no other config is declared at the same height
	DECLARE_STATEFUL_VALIDATOR(CatapultConfig, model::CatapultBlockChainConfigNotification<1>)(uint32_t maxBlockChainConfigSize);
}}
