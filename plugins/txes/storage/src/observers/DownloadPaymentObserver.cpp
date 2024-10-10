/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DownloadPaymentNotification<1>;

	DECLARE_OBSERVER(DownloadPayment, Notification)(const std::shared_ptr<state::StorageState>& pStorageState) {
		return MAKE_OBSERVER(DownloadPayment, Notification, ([pStorageState](const Notification& notification, ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadPayment)");

			auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
			auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
			auto& downloadChannelEntry = downloadChannelIter.get();

			downloadChannelEntry.increaseDownloadSize(notification.DownloadSizeMegabytes);

			auto pChannel = utils::GetDownloadChannel(pStorageState->replicatorKey(), downloadChannelEntry);
			if (pChannel)
				context.Notifications.push_back(std::make_unique<model::DownloadPaymentServiceNotification<1>>(std::move(pChannel)));
		}))
	}
}}
