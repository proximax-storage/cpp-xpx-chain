/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ReplicatorOnboardingNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(ReplicatorOnboarding, [](const Notification& notification, const ValidatorContext& context) {
	  	const auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

		if (replicatorCache.contains(notification.PublicKey))
			return Failure_Storage_Replicator_Already_Registered;

		if (utils::FileSize::FromMegabytes(notification.Capacity.unwrap()) < pluginConfig.MinCapacity)
			return Failure_Storage_Replicator_Capacity_Insufficient;

		return ValidationResult::Success;
	});

}}
