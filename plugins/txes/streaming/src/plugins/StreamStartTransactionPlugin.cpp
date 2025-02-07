/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <src/utils/StreamingUtils.h>
#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "StreamStartTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "catapult/model/StreamingNotifications.h"
#include "src/model/StreamStartTransaction.h"
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
					auto transactionHash = CalculateHash(transaction, config.GenerationHash);
					std::string folderName((const char*)transaction.FolderNamePtr(), transaction.FolderNameSize);
					sub.notify(StreamStartNotification<1>(
							transactionHash,
							transaction.DriveKey,
							transaction.Signer,
							transaction.ExpectedUploadSizeMegabytes,
							folderName
					));
					sub.notify(StreamStartFolderNameNotification<1>(
							transaction.FolderNameSize
					));

					const auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, config.NetworkIdentifier));
					const auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);
					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

					sub.notify(BalanceTransferNotification<1>(
							transaction.Signer, driveAddress, currencyMosaicId, transaction.FeedbackFeeAmount));
					const auto pStreamingWork = sub.mempool().malloc(model::StreamingWork(transaction.DriveKey, transaction.ExpectedUploadSizeMegabytes));
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
					CATAPULT_LOG(debug) << "invalid version of StreamStartTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StreamStart, Default, CreatePublisher, config::ImmutableConfiguration)
}}
