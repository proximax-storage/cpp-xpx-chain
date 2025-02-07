/**
*** Copyright 2025 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/BcDriveCache.h"
#include "plugins/txes/streaming/src/validators/Validators.h"

namespace catapult { namespace validators {

	using Notification = model::UpdateDriveSizeNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(UpdateDriveSize, [](const Notification& notification, const ValidatorContext& context) {
		const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		const auto driveIter = driveCache.find(notification.DriveKey);
		const auto& pDriveEntry = driveIter.tryGet();

		// Check if respective drive exists
		if (!pDriveEntry)
			return Failure_Storage_Drive_Not_Found;

		// Check if the signer is the owner
		const auto& owner = pDriveEntry->owner();
		if (owner != notification.DriveOwner)
			return Failure_Storage_Is_Not_Owner;

		// Check if the new drive size is less than the current size
		if (notification.NewDriveSize >= pDriveEntry->size())
			return Failure_Storage_Drive_Size_Is_Not_Decreased;

		// Check if the new drive size >= minDriveSize
		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();
		if (utils::FileSize::FromMegabytes(notification.NewDriveSize) < pluginConfig.MinDriveSize)
			return Failure_Storage_Drive_Size_Insufficient;

		return ValidationResult::Success;
	});
}}
