/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DownloadPayment, model::DownloadPaymentNotification<1>, [](const model::DownloadPaymentNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadPayment)");

	  	auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
	  	auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
	  	auto& downloadChannelEntry = downloadChannelIter.get();

		downloadChannelEntry.increaseDownloadSize(notification.DownloadSizeMegabytes);
	});
}}
