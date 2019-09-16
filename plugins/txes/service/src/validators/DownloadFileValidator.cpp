/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/FileCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::DownloadFileNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(DownloadFile, [](const auto& notification, const ValidatorContext& context) {
		const auto& fileCache = context.Cache.sub<cache::FileCache>();
		if (fileCache.contains(state::MakeDriveFileKey(notification.File.Drive, notification.File.Hash)))
            return Failure_Service_Drive_File_Doesnt_Exist;

		return ValidationResult::Success;
	});
}}
