/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "VerificationPaymentTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/VerificationPaymentTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(VerificationPaymentNotification<1>(
							transaction.Signer,
							transaction.DriveKey
					));

					const auto driveAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, config.NetworkIdentifier));
					const auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);

					sub.notify(BalanceTransferNotification<1>(
							transaction.Signer, driveAddress, currencyMosaicId, transaction.VerificationFeeAmount));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of VerificationPaymentTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

		DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(VerificationPayment, Default, CreatePublisher, config::ImmutableConfiguration)
}}
