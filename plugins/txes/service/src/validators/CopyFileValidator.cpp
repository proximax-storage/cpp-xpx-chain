/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/FileCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::CopyFileNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(CopyFile, [](const auto& notification, const ValidatorContext& context) {
		if (notification.Source.Drive != notification.Destination.Drive)
            return Failure_Service_Desitination_And_Source_Are_From_Different_Drives;

		const auto& fileCache = context.Cache.sub<cache::FileCache>();
		if ((notification.Destination.ParentHash != Hash256()) && !fileCache.contains(state::MakeDriveFileKey(notification.Destination.Drive, notification.Destination.ParentHash)))
            return Failure_Service_Drive_Parent_Directory_Doesnt_Exist;

		if (!fileCache.contains(state::MakeDriveFileKey(notification.Source.Drive, notification.Source.Hash)))
            return Failure_Service_Drive_File_Doesnt_Exist;

		if (fileCache.contains(state::MakeDriveFileKey(notification.Destination.Drive, notification.Destination.Hash)))
            return Failure_Service_Drive_File_Exists;

		return ValidationResult::Success;
	});
}}
