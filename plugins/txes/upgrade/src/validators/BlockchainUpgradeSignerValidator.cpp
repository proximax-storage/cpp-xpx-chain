/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/StatefulValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::BlockchainUpgradeSignerNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(BlockchainUpgradeSigner, [](const auto& notification, const StatefulValidatorContext& context) {
		if (notification.Signer != context.Network.PublicKey)
			return Failure_BlockchainUpgrade_Invalid_Signer;

		return ValidationResult::Success;
	});
}}
