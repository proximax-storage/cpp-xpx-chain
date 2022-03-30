/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DownloadChannelRefundNotification<1>;

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DownloadChannelRefund, Notification, [&liquidityProvider](const Notification& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadChannelRefund)");

	  	auto& downloadChannelCache = context.Cache.sub<cache::DownloadChannelCache>();
	  	auto downloadChannelIter = downloadChannelCache.find(notification.DownloadChannelId);
	  	auto& downloadChannelEntry = downloadChannelIter.get();

		if (downloadChannelEntry.downloadApprovalCountLeft() > 0 && !downloadChannelEntry.isFinishPublished()) {
			// THe download channel continues to work, so no refund is needed
			return;
		}

		const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto senderIter = accountStateCache.find(Key(notification.DownloadChannelId.array()));
	  	const auto& senderState = senderIter.get();

	  	const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;

		// Refunding streaming mosaics.
		const auto& streamingRefundAmount = senderState.Balances.get(streamingMosaicId);
		liquidityProvider.debitMosaics(context, downloadChannelEntry.id().array(), downloadChannelEntry.consumer(), config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), streamingRefundAmount);

	  	auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		for (const auto& replicatorKey: downloadChannelEntry.shardReplicators()) {
			auto& replicatorEntry = replicatorCache.find(replicatorKey).get();
			replicatorEntry.downloadChannels().erase(notification.DownloadChannelId);
		}
		downloadChannelCache.remove(notification.DownloadChannelId);

		// TODO: Add currency refunding
	})
}}
