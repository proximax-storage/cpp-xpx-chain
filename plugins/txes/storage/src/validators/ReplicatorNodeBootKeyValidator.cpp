/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BootKeyReplicatorCache.h"

namespace catapult { namespace validators {

	using Notification = model::ReplicatorNodeBootKeyNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ReplicatorNodeBootKey, ([](const Notification& notification, const ValidatorContext& context) {
		const auto& cache = context.Cache.sub<cache::BootKeyReplicatorCache>();
		if (cache.contains(notification.NodeBootKey))
			return Failure_Storage_Boot_Key_Is_Registered_With_Other_Replicator;

		return ValidationResult::Success;
	}));
}}
