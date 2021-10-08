/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(StreamStart, model::StreamStartNotification<1>, [](const model::StreamStartNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModification)");

	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();

		auto& activeDataModifications = driveEntry.activeDataModifications();
		activeDataModifications.emplace_back(state::ActiveDataModification(
			notification.StreamId,
			notification.Owner,
			notification.ExpectedUploadSize,
			notification.Folder
		));
	});
}}
