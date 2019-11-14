/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "StartDriveVerificationTransactionPlugin.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/StartDriveVerificationTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "sdk/src/extensions/ConversionExtensions.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction &transaction, const Height&, NotificationSubscriber &sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(StartDriveVerificationNotification<1>(
							transaction.DriveKey,
							transaction.Signer,
							transaction.VerificationFee
						));

						sub.notify(BalanceTransferNotification<1>(
							transaction.Signer,
							extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, config.NetworkIdentifier)),
							config::GetUnresolvedStreamingMosaicId(config),
							transaction.VerificationFee
						));

						break;
					}
					default:
						CATAPULT_LOG(debug) << "invalid version of StartDriveVerificationTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StartDriveVerification, Default, CreatePublisher, config::ImmutableConfiguration)
}}
