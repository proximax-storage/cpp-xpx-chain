/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DownloadApprovalNotification<1>;

	DECLARE_OBSERVER(DownloadApproval, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(DownloadApproval, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadApproval)");

			auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
			auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
			auto& downloadChannelEntry = downloadChannelIter.get();
			auto pChannel = utils::GetDownloadChannel(pStorageState->replicatorKey(), downloadChannelEntry);
			downloadChannelEntry.downloadApprovalInitiationEvent().reset();

			if (pChannel)
				context.Notifications.push_back(std::make_unique<model::DownloadApprovalServiceNotification<1>>(std::move(pChannel), false));
		}))
	}
}}
