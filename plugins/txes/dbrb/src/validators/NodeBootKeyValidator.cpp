/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DbrbViewCache.h"


namespace catapult { namespace validators {

	using Notification = model::ReplicatorNodeBootKeyNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(NodeBootKey, ([](const Notification& notification, const ValidatorContext& context) {
		const auto& cache = context.Cache.sub<cache::DbrbViewCache>();
		const auto& bootstrapProcesses = context.Config.Network.DbrbBootstrapProcesses;
		if (!cache.contains(notification.NodeBootKey) && (bootstrapProcesses.find(notification.NodeBootKey) == bootstrapProcesses.cend()))
			return Failure_Dbrb_Process_Is_Not_Registered;

		return ValidationResult::Success;
	}));
}}
