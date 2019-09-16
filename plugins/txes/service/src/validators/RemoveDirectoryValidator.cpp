/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/FileCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::RemoveDirectoryNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(RemoveDirectory, [](const auto& notification, const ValidatorContext& context) {
		const auto& fileCache = context.Cache.sub<cache::FileCache>();
		if (fileCache.contains(state::MakeDriveFileKey(notification.File.Drive, notification.File.Hash)))
            return Failure_Service_Drive_Directory_Doesnt_Exist;

		return ValidationResult::Success;
	});
}}
