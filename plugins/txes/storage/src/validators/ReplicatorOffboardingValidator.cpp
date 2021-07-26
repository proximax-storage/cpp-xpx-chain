/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ReplicatorOffboardingNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ReplicatorOffboarding, [](const Notification& notification, const ValidatorContext& context) {
	  	auto replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
	  	auto replicatorIter = replicatorCache.find(notification.PublicKey);
		const auto& pReplicatorEntry = replicatorIter.tryGet();
		if (!replicatorCache.contains(notification.PublicKey))
			return Failure_Storage_Replicator_Not_Registered;

		return ValidationResult::Success;
	});

}}
