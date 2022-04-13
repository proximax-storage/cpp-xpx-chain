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
			// The download channel continues to work, so no refund is needed
			return;
		}

		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto senderIter = accountStateCache.find(Key(notification.DownloadChannelId.array()));
	  	auto& senderState = senderIter.get();
		auto recipientIter = accountStateCache.find(downloadChannelEntry.consumer());
	  	auto& recipientState = recipientIter.get();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
	  	const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;

		// Refunding currency and streaming mosaics.
	  	const auto& currencyRefundAmount = senderState.Balances.get(currencyMosaicId);
		const auto& streamingRefundAmount = senderState.Balances.get(streamingMosaicId);
	  	recipientState.Balances.credit(currencyMosaicId, currencyRefundAmount);
		senderState.Balances.debit(currencyMosaicId, currencyRefundAmount);
		liquidityProvider.debitMosaics(context, downloadChannelEntry.id().array(), downloadChannelEntry.consumer(), config::GetUnresolvedStreamingMosaicId(context.Config.Immutable), streamingRefundAmount);

	  	// Removing associations with the download channel in respective drive and replicator entries.
	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto& driveEntry = driveCache.find(downloadChannelEntry.drive()).get();
	  	driveEntry.downloadShards().erase(notification.DownloadChannelId);

		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		for (const auto& replicatorKey: downloadChannelEntry.shardReplicators()) {
			auto& replicatorEntry = replicatorCache.find(replicatorKey).get();
			replicatorEntry.downloadChannels().erase(notification.DownloadChannelId);
		}

	  	// Removing download channel entry from the cache.
		downloadChannelCache.remove(notification.DownloadChannelId);
	})
}}
