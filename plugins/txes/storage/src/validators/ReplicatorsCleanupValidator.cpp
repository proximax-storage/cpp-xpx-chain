/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/ReplicatorCache.h"

namespace catapult { namespace validators {

	using Notification = model::ReplicatorsCleanupNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ReplicatorsCleanup, ([](const Notification& notification, const ValidatorContext& context) {
		if (context.NetworkIdentifier == model::NetworkIdentifier::Public)
			return Failure_Storage_Replicator_Cleanup_Is_Unallowed_In_Public_Network;

		const auto& cache = context.Cache.sub<cache::ReplicatorCache>();
		auto pReplicatorKey = notification.ReplicatorKeysPtr;
		for (auto i = 0u; i < notification.ReplicatorCount; ++i, ++pReplicatorKey) {
			auto iter = cache.find(*pReplicatorKey);
			auto pEntry = iter.tryGet();
			if (!pEntry)
				return Failure_Storage_Replicator_Not_Found;

			if (pEntry->nodeBootKey() != Key())
				return Failure_Storage_Replicator_Is_Bound_With_Boot_Key;
		}

		return ValidationResult::Success;
	}));
}}
