/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "FinishDownloadTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/FinishDownloadTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
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

					auto txHash = CalculateHash(transaction, config.GenerationHash).array();

					sub.notify(FinishDownloadNotification<1>(
							transaction.Signer,
							transaction.DownloadChannelId,
							txHash
					));

					const auto downloadChannelAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(Key(transaction.DownloadChannelId.array()), config.NetworkIdentifier));
					const auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);

					sub.notify(BalanceTransferNotification<1>(
							transaction.Signer, downloadChannelAddress, currencyMosaicId, transaction.FeedbackFeeAmount));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of FinishDownloadTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(FinishDownload, Default, CreatePublisher, config::ImmutableConfiguration)
}}
