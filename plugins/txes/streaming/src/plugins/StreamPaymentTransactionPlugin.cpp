/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/utils/StreamingUtils.h>
#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "StreamPaymentTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "plugins/txes/streaming/src/model/StreamingNotifications.h"
#include "src/model/StreamPaymentTransaction.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/NotificationSubscriber.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [&config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
					sub.notify(StreamPaymentNotification<1>(
							transaction.DriveKey,
							transaction.StreamId,
							transaction.AdditionalUploadSizeMegabytes));

					const auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, config.NetworkIdentifier));
					const auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);
					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

					const auto pStreamingWork = sub.mempool().malloc(model::StreamingWork(transaction.DriveKey, transaction.AdditionalUploadSizeMegabytes));
					utils::SwapMosaics(
							transaction.Signer,
							transaction.DriveKey,
							{ std::make_pair(streamingMosaicId, UnresolvedAmount(0, UnresolvedAmountType::StreamingWork, pStreamingWork)) },
							sub,
							config,
							utils::SwapOperation::Buy);

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of StreamPaymentTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StreamPayment, Default, CreatePublisher, config::ImmutableConfiguration)
}}
