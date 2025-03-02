/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/ReplicatorCache.h"

namespace catapult { namespace validators {

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(ReplicatorsCleanupV1, model::ReplicatorsCleanupNotification<1>, ([](const model::ReplicatorsCleanupNotification<1>& notification, const ValidatorContext& context) {
		if (!notification.ReplicatorCount)
			return Failure_Storage_No_Replicators_To_Remove;

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

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(ReplicatorsCleanupV2, model::ReplicatorsCleanupNotification<2>, ([](const model::ReplicatorsCleanupNotification<2>& notification, const ValidatorContext& context) {
		if (!notification.ReplicatorCount)
			return Failure_Storage_No_Replicators_To_Remove;

		const auto& cache = context.Cache.sub<cache::ReplicatorCache>();
		auto pReplicatorKey = notification.ReplicatorKeysPtr;
		for (auto i = 0u; i < notification.ReplicatorCount; ++i, ++pReplicatorKey) {
			auto iter = cache.find(*pReplicatorKey);
			auto pEntry = iter.tryGet();
			if (!pEntry)
				return Failure_Storage_Replicator_Not_Found;
		}

		return ValidationResult::Success;
	}));
}}
