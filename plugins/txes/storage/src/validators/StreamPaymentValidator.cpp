/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::StreamPaymentNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(StreamPayment, [](const Notification& notification, const ValidatorContext& context) {
	  	auto driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

	  	// Check if stream size >= maxModificationSize
	  	if (utils::FileSize::FromMegabytes(notification.AdditionalUploadSize) > pluginConfig.MaxModificationSize)
	  		return Failure_Storage_Upload_Size_Excessive;

	  	auto driveIter = driveCache.find(notification.DriveKey);
		const auto& pDriveEntry = driveIter.tryGet();
		if (!pDriveEntry)
			return Failure_Storage_Drive_Not_Found;

		auto pModification = std::find_if(
				pDriveEntry->activeDataModifications().begin(),
				pDriveEntry->activeDataModifications().end(),
				[notification] (const state::ActiveDataModification& modification) -> bool {
					return notification.StreamId == modification.Id;
				});
		if (pModification == pDriveEntry->activeDataModifications().end()) {
			return Failure_Storage_Invalid_Stream_Id;
		}

		if (pModification->ReadyForApproval) {
			return Failure_Storage_Stream_Already_Finished;
		}

		return ValidationResult::Success;
	});

}}
