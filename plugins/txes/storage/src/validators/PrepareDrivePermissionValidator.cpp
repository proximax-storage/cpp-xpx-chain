/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace validators {

	using Notification = model::PrepareDriveNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(PrepareDrivePermission, [](const model::PrepareDriveNotification<1> &notification, const ValidatorContext& context) {

		const auto& driveCache = context.Cache.sub<cache::DriveCache>();
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

		// Checking if drive size >= minDriveSize
		// TODO: Check conversion
		if (utils::FileSize::FromMegabytes(notification.DriveSize.unwrap()) < pluginConfig.MinDriveSize)
			return Failure_Storage_Prepare_Too_Small_DriveSize;

		// Check if number of replicators >= minReplicatorCount
		if (notification.ReplicatorCount < pluginConfig.MinReplicatorCount)
			return Failure_Storage_Prepare_Too_Small_ReplicatorCount;

		return ValidationResult::Success;
	});

}}
