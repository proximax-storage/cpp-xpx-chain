/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::FinishDownloadNotification<1>;

	DECLARE_OBSERVER(FinishDownload, Notification)() {
		return MAKE_OBSERVER(FinishDownload, Notification, ([](const Notification& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (PrepareDrive)");

			auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
			auto& downloadEntry = downloadCache.find(notification.DownloadChannelId).get();

			downloadEntry.setFinishPublished(true);
			downloadEntry.downloadApprovalInitiationEvent() = notification.TransactionHash;
		}))
	}
}}
