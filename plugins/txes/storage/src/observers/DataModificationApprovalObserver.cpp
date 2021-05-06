/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DataModificationApproval, model::DataModificationApprovalNotification<1>, [](const model::DataModificationApprovalNotification<1>& notification, ObserverContext& context) {

	  	auto& driveEntry = context.Cache.sub<cache::DriveCache>().find(notification.DriveKey).get();
		auto& activeDataModifications = driveEntry.activeDataModifications();
		auto& finishedDataModifications = driveEntry.finishedDataModifications();

		if (NotifyMode::Commit == context.Mode) {
			activeDataModifications.erase(activeDataModifications.begin());
			finishedDataModifications.emplace_back(notification.CallReference, state::DataModificationState::Succeeded);
		} else {
			finishedDataModifications.erase(finishedDataModifications.end());
			activeDataModifications.insert(activeDataModifications.begin(), notification.CallReference);
		}

	});
}}
