/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/CatapultUpgradeNotifications.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {
	/// A validator implementation that applies to catapult upgrade signer notification and validates that:
	/// - signer is nemesis account
	DECLARE_STATEFUL_VALIDATOR(CatapultUpgradeSigner, model::CatapultUpgradeSignerNotification<1>)();

	/// A validator implementation that applies to catapult upgrade notification and validates that:
	/// - upgrade period is valid (greater or equal the minimum value set in config)
	/// - no other upgrade is declared at the same height
	DECLARE_STATEFUL_VALIDATOR(CatapultUpgrade, model::CatapultUpgradeVersionNotification<1>)(uint16_t minUpgradePeriod);
}}
