/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "StoragePaymentTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/StoragePaymentTransaction.h"
#include "src/utils/StorageUtils.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					const auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, config.NetworkIdentifier));
					const auto storageMosaicId = config::GetUnresolvedStorageMosaicId(config);

					utils::SwapMosaics(transaction.Signer, { { storageMosaicId, transaction.StorageUnits } }, sub, config, utils::SwapOperation::Buy);
					sub.notify(BalanceTransferNotification<1>(
							transaction.Signer, driveAddress, storageMosaicId, transaction.StorageUnits));

					sub.notify(StoragePaymentNotification<1>(
							transaction.Signer,
							transaction.DriveKey,
							transaction.StorageUnits
					));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of StoragePaymentTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StoragePayment, Default, CreatePublisher, config::ImmutableConfiguration)
}}
