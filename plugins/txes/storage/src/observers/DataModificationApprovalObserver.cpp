/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DataModificationApproval, model::DataModificationApprovalNotification<1>, [](const model::DataModificationApprovalNotification<1>& notification, ObserverContext& context) {

	  	auto& driveCache = context.Cache.sub<cache::DriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();

		auto& activeDataModifications = driveEntry.activeDataModifications();
		auto& completedDataModifications = driveEntry.completedDataModifications();

		if (NotifyMode::Commit == context.Mode) {
			activeDataModifications.erase(activeDataModifications.begin());
			completedDataModifications.emplace_back(notification.CallReference, state::DataModificationState::Succeeded);
		} else {
			completedDataModifications.erase(completedDataModifications.end());
			activeDataModifications.insert(activeDataModifications.begin(), notification.CallReference);
		}

	});
}}
