/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DeploySupercontractNotification<1>;

	DECLARE_OBSERVER(DeploySupercontract, Notification)() {
		return MAKE_OBSERVER(DeploySupercontract, Notification, ([](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DeploySupercontract)");

			auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
			state::BcDriveEntry driveEntry(notification.DriveKey);
			driveEntry.setSupercontractIsDeployed(true);
			driveCache.insert(driveEntry);
		}))
	}
}}