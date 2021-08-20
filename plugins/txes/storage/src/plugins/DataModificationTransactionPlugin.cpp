/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "DataModificationTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/DataModificationTransaction.h"
#include "src/utils/StorageUtils.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/EntityHasher.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [&config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					auto transactionHash = CalculateHash(transaction, config.GenerationHash);
					sub.notify(DataModificationNotification<1>(
							transactionHash,
							transaction.DriveKey,
							transaction.Signer,
							transaction.DownloadDataCdi,
							transaction.UploadSize));

					const auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, config.NetworkIdentifier));
					const auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);
					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

					sub.notify(BalanceTransferNotification<1>(
							transaction.Signer, driveAddress, currencyMosaicId, transaction.FeedbackFeeAmount));
					const auto pStreamingWork = sub.mempool().malloc(model::StreamingWork(transaction.DriveKey, transaction.UploadSize));
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
					CATAPULT_LOG(debug) << "invalid version of DataModificationTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(DataModification, Default, CreatePublisher, config::ImmutableConfiguration)
}}
