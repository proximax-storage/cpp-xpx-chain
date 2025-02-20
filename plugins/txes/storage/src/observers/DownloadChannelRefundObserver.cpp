/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/catapult/model/Address.h"
#include "src/utils/Queue.h"

namespace catapult { namespace observers {

	using Notification = model::DownloadChannelRefundNotification<1>;

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DownloadChannelRefund, Notification, [&liquidityProvider](const Notification& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadChannelRefund)");

		auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		auto downloadChannelIter = downloadCache.find(notification.DownloadChannelId);
	  	auto& downloadChannelEntry = downloadChannelIter.get();

		if (!downloadChannelEntry.isCloseInitiated()) {
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
	  	auto& statementBuilder = context.StatementBuilder();

	  	// Refunding currency mosaics.
	  	const auto& currencyRefundAmount = senderState.Balances.get(currencyMosaicId);
		senderState.Balances.debit(currencyMosaicId, currencyRefundAmount, context.Height);
		recipientState.Balances.credit(currencyMosaicId, currencyRefundAmount, context.Height);

	  	// Adding Refund receipt for currency.
		{
			const auto receiptType = model::Receipt_Type_Download_Channel_Refund;
			const model::StorageReceipt receipt(receiptType, downloadChannelEntry.id().array(),
												downloadChannelEntry.consumer(),
												{ currencyMosaicId, currencyMosaicId }, currencyRefundAmount);
			statementBuilder.addTransactionReceipt(receipt);
		}

		// Refunding streaming mosaics.
		const auto& streamingRefundAmount = senderState.Balances.get(streamingMosaicId);
		liquidityProvider->debitMosaics(context, downloadChannelEntry.id().array(), downloadChannelEntry.consumer(),
									   config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
									   streamingRefundAmount);

	  	// Adding Refund receipt for streaming mosaic.
		{
			const auto receiptType = model::Receipt_Type_Download_Channel_Refund;
			const model::StorageReceipt receipt(receiptType, downloadChannelEntry.id().array(),
												downloadChannelEntry.consumer(),
												{ streamingMosaicId, currencyMosaicId }, streamingRefundAmount);
			statementBuilder.addTransactionReceipt(receipt);
		}

	  	// Removing associations with the download channel in respective drive and replicator entries.
	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIt = driveCache.find(downloadChannelEntry.drive());
	  	auto* pDriveEntry = driveIt.tryGet();
		if (pDriveEntry) {
			pDriveEntry->downloadShards().erase(notification.DownloadChannelId);
		}

		auto& replicatorCache = context.Cache.sub<cache::ReplicatorCache>();
		for (const auto& replicatorKey: downloadChannelEntry.shardReplicators()) {
			auto replicatorIt = replicatorCache.find(replicatorKey);
			auto& replicatorEntry = replicatorIt.get();
			replicatorEntry.downloadChannels().erase(notification.DownloadChannelId);
		}

		downloadCache.remove(notification.DownloadChannelId);
		// TODO: Add currency refunding
	})
}}
