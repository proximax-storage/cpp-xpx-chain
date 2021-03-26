/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/licensing/LicenseManager.h"

namespace catapult { namespace validators {

	using Notification = model::BlockNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(License, Notification)(const std::shared_ptr<licensing::LicenseManager>& pLicenseManager) {
		return MAKE_STATEFUL_VALIDATOR(License, [pLicenseManager](const auto& notification, const auto& context) {
			if (context.Height > Height(1) && !pLicenseManager->blockAllowedAt(context.Height, notification.StateHash)) {
				return Failure_Core_License_Invalid_Or_Expired;
			}
			return ValidationResult::Success;
		});
	};
}}
