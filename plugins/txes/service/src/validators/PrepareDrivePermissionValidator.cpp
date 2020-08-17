/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace validators {
	namespace {
		template<VersionType version>
		ValidationResult handler_v1(const model::PrepareDriveNotification<version> &notification, const ValidatorContext& context) {
			const auto& driveCache = context.Cache.sub<cache::DriveCache>();
			if (driveCache.contains(notification.DriveKey))
				return Failure_Service_Drive_Already_Exists;

			return ValidationResult::Success;
		}
	}

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(PrepareDrivePermissionV1, model::PrepareDriveNotification<1>, handler_v1<1>)
	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(PrepareDrivePermissionV2, model::PrepareDriveNotification<2>, handler_v1<2>)

}}
