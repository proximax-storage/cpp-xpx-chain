/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

    DEFINE_OBSERVER(DriveClosure, model::DriveClosureNotification<1>, ([](const auto& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DriveClosure)");

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		for (const auto& replicatorKey : driveEntry.replicators()) {
			auto replicatorIter = replicatorCache.find(replicatorKey);
			auto& replicatorEntry = replicatorIter.get();
			replicatorEntry.drives().erase(replicatorEntry.drives().find(notification.DriveKey));
		}

		auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
		for (const auto& activeDownload : driveEntry.activeDownloads()) {
			auto downloadChannelIter = downloadChannelCache.find(activeDownload);
			auto& downloadChannelEntry = downloadChannelIter.get();
			downloadChannelCache.remove(downloadChannelEntry.id());
		}
		
		driveCache.remove(notification.DriveKey);
	}));
}}
