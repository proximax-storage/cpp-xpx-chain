/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/FileCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::CreateDirectoryNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(CreateDirectory, [](const auto& notification, const ValidatorContext& context) {
		const auto& fileCache = context.Cache.sub<cache::FileCache>();
		if ((notification.File.ParentHash != Hash256()) && !fileCache.contains(state::MakeDriveFileKey(notification.File.Drive, notification.File.ParentHash)))
            return Failure_Service_Drive_Parent_Directory_Doesnt_Exist;

		if (fileCache.contains(state::MakeDriveFileKey(notification.File.Drive, notification.File.Hash)))
            return Failure_Service_Drive_File_Exists;

		return ValidationResult::Success;
	});
}}
