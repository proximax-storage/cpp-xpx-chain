/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DownloadChannel, model::DownloadNotification<1>, [](const model::DownloadNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadChannel)");

	  	auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		state::DownloadChannelEntry downloadEntry(notification.Id);
		downloadEntry.setConsumer(notification.Consumer);
		downloadEntry.setFeedbackFeeAmount(notification.FeedbackFeeAmount);
		// TODO: Buy storage units for xpx in notification.DownloadSize
		downloadEntry.setDownloadSize(notification.DownloadSize);

		auto pKey = notification.WhitelistedPublicKeysPtr;
		for (auto i = 0; i < notification.WhitelistedPublicKeyCount; ++i, ++pKey)
			downloadEntry.whitelistedPublicKeys().push_back(*pKey);

		downloadCache.insert(downloadEntry);
	});
}}
