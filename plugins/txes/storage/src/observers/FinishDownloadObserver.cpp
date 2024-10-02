/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include <src/utils/Queue.h>

namespace catapult { namespace observers {

	using Notification = model::FinishDownloadNotification<1>;

	DECLARE_OBSERVER(FinishDownload, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(FinishDownload, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (FinishDownload)");

			auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
			auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
			auto& downloadChannelEntry = downloadChannelIter.get();

			downloadChannelEntry.setFinishPublished(true);

			auto& queueCache = context.Cache.template sub<cache::QueueCache>();
			utils::QueueAdapter<cache::DownloadChannelCache> downloadQueueAdapter(queueCache, state::DownloadChannelPaymentQueueKey, downloadChannelCache);
			downloadQueueAdapter.remove(downloadChannelEntry.entryKey());

			downloadChannelEntry.downloadApprovalInitiationEvent() = notification.TransactionHash;

			auto pChannel = utils::GetDownloadChannel(pStorageState->replicatorKey(), downloadChannelEntry);
			if (pChannel)
				context.Notifications.push_back(std::make_unique<model::DownloadRewardServiceNotification<1>>(std::vector<std::shared_ptr<state::DownloadChannel>>{ std::move(pChannel) }));
		}))
	}
}}
