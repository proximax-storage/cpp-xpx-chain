/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include <src/utils/Queue.h>

namespace catapult { namespace observers {

	DEFINE_OBSERVER(FinishDownload, model::FinishDownloadNotification<1>, [](const model::FinishDownloadNotification<1>& notification, ObserverContext& context) {
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
	});
}}
