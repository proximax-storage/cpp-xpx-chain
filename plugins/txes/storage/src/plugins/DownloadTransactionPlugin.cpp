/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "DownloadTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
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
//					auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);
//					auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, config.NetworkIdentifier));
//					sub.notify(BalanceTransferNotification<1>(
//							transaction.Signer, driveAddress, currencyMosaicId, transaction.TransactionFee));
//					sub.notify(BalanceTransferNotification<1>(
//							transaction.Signer, driveAddress, currencyMosaicId, Amount(transaction.DownloadSize)));
//					sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
//					utils::SwapMosaics(transaction.Signer, { { config::GetUnresolvedStorageMosaicId(config), Amount(transaction.DownloadSize) } }, sub, config, utils::SwapOperation::Buy);

					auto downloadChannelId = CalculateHash(transaction, config.GenerationHash);
					sub.notify(DownloadNotification<1>(
							downloadChannelId,
							transaction.DriveKey,
							transaction.Signer,
							transaction.DownloadSize,
							transaction.TransactionFee));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of DownloadChannelTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(Download, Default, CreatePublisher, config::ImmutableConfiguration)
}}
