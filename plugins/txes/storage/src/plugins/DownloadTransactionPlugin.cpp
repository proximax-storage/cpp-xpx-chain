/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "DownloadTransactionPlugin.h"
#include "src/model/StorageNotifications.h"
#include "src/model/DownloadTransaction.h"
#include "src/utils/StorageUtils.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/EntityHasher.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					const auto downloadChannelId = CalculateHash(transaction, config.GenerationHash);
					const auto downloadChannelAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(Key(downloadChannelId.array()), config.NetworkIdentifier));
					const auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);
					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

					sub.notify(BalanceTransferNotification<1>(
							transaction.Signer, downloadChannelAddress, currencyMosaicId, transaction.FeedbackFeeAmount));
					utils::SwapMosaics(transaction.Signer, { { streamingMosaicId, Amount(transaction.DownloadSize) } }, sub, config, utils::SwapOperation::Buy);
					sub.notify(BalanceTransferNotification<1>(
							transaction.Signer, downloadChannelAddress, streamingMosaicId, Amount(transaction.DownloadSize)));
					sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
					sub.notify(DownloadNotification<1>(
							downloadChannelId,
							transaction.DriveKey,
							transaction.Signer,
							transaction.DownloadSize,
							transaction.FeedbackFeeAmount));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of DownloadTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(Download, Default, CreatePublisher, config::ImmutableConfiguration)
}}
