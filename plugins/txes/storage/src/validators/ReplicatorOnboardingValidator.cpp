/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ReplicatorOnboardingNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ReplicatorOnboarding, [](const Notification& notification, const ValidatorContext& context) {
	  	auto replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		if (replicatorCache.contains(notification.PublicKey))
			return Failure_Storage_Replicator_Already_Registered;

		auto blsKeysCache = context.Cache.sub<cache::BlsKeysCache>();
		if (blsKeysCache.contains(notification.BlsKey))
			  return Failure_Storage_BLS_Key_Already_Registered;

		return ValidationResult::Success;
	});

}}
