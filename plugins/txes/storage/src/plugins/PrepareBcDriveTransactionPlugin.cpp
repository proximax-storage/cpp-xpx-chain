/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "PrepareBcDriveTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/PrepareBcDriveTransaction.h"
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
					auto driveKey = Key(CalculateHash(transaction, config.GenerationHash).array());
					sub.notify(PrepareDriveNotification<1>(
							transaction.Signer,
							driveKey,
							transaction.DriveSize,
							transaction.VerificationFeeAmount,
							transaction.ReplicatorCount
					));

					const auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(driveKey, config.NetworkIdentifier));
					const auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);
					const auto storageMosaicId = config::GetUnresolvedStorageMosaicId(config);
					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

					sub.notify(BalanceTransferNotification<1>(
							transaction.Signer, driveAddress, currencyMosaicId, transaction.VerificationFeeAmount));
					utils::SwapMosaics(
							transaction.Signer,
							driveKey,
							{ {storageMosaicId, Amount(transaction.DriveSize * transaction.ReplicatorCount)} },
							sub,
							config,
							utils::SwapOperation::Buy);
					utils::SwapMosaics(
							transaction.Signer,
							driveKey,
							{ {streamingMosaicId, Amount(2 * transaction.DriveSize * transaction.ReplicatorCount)} },
							sub,
							config,
							utils::SwapOperation::Buy);

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of PrepareBcDriveTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(PrepareBcDrive, Default, CreatePublisher, config::ImmutableConfiguration)
}}
