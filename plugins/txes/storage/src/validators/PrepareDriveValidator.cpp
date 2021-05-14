/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/config/StorageConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::PrepareDriveNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(PrepareDrive, [](const model::PrepareDriveNotification<1> &notification, const ValidatorContext& context) {

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

		// Check if drive size >= minDriveSize
		// TODO: Check conversion
		if (utils::FileSize::FromMegabytes(notification.DriveSize) < pluginConfig.MinDriveSize)
			return Failure_Storage_Drive_Size_Insufficient;

		// Check if number of replicators >= minReplicatorCount
		if (notification.ReplicatorCount < pluginConfig.MinReplicatorCount)
			return Failure_Storage_Replicator_Count_Insufficient;

	  	// Check if the drive already exists
	  	if (driveCache.contains(notification.DriveKey))
			return Failure_Storage_Drive_Already_Exists;

		return ValidationResult::Success;
	});

}}
