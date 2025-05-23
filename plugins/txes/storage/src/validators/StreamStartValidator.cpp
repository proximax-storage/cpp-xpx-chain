/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::StreamStartNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(StreamStart, [](const Notification& notification, const ValidatorContext& context) {
	  	auto driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

	  	// Check if stream size >= maxModificationSize
	  	if (utils::FileSize::FromMegabytes(notification.ExpectedUploadSize) > pluginConfig.MaxModificationSize)
	  		return Failure_Storage_Upload_Size_Excessive;

	  	auto driveIter = driveCache.find(notification.DriveKey);
		const auto& pDriveEntry = driveIter.tryGet();
		if (!pDriveEntry)
			return Failure_Storage_Drive_Not_Found;

		const auto& owner = pDriveEntry->owner();
		if (owner != notification.Owner) {
			return Failure_Storage_Is_Not_Owner;
		}

		const auto& activeDataModifications = pDriveEntry->activeDataModifications();
	  	for (const auto& modification : activeDataModifications) {
	  		if (modification.Id == notification.StreamId)
				return Failure_Storage_Stream_Already_Exists;
	  	}

		return ValidationResult::Success;
	});

}}
