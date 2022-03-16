/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DownloadChannelRefundNotification<1>;

	DEFINE_OBSERVER(DownloadChannelRefund, Notification, [](const Notification& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadChannelRefund)");

	  	auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
	  	auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
	  	auto& downloadChannelEntry = downloadChannelIter.get();

		if (downloadChannelEntry.downloadApprovalCountLeft() > 0 && !downloadChannelEntry.isFinishPublished()) {
			// THe download channel continues to work, so no refund is needed
			return;
		}

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto senderIter = accountStateCache.find(Key(notification.DownloadChannelId.array()));
	  	auto& senderState = senderIter.get();
	  	auto recipientIter = accountStateCache.find(downloadChannelEntry.consumer());
	  	auto& recipientState = recipientIter.get();

	  	const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
	  	const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;

		// Refunding streaming mosaics.
		const auto& streamingRefundAmount = senderState.Balances.get(streamingMosaicId);
	  	senderState.Balances.debit(streamingMosaicId, streamingRefundAmount, context.Height);
	  	recipientState.Balances.credit(currencyMosaicId, streamingRefundAmount, context.Height);

		downloadChannelCache.remove(notification.DownloadChannelId);
		// TODO: Remove channel from Replicator's Cache

		// TODO: Add currency refunding
	})
}}
