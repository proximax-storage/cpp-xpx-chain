/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "StartDriveVerificationTransactionPlugin.h"
#include "src/config/ServiceConfiguration.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/StartDriveVerificationTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "plugins/txes/lock_secret/src/model/SecretLockNotifications.h"
#include "sdk/src/extensions/ConversionExtensions.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			return [pConfigHolder](const TTransaction &transaction, const Height& associatedHeight, NotificationSubscriber &sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
						sub.notify(StartDriveVerificationNotification<1>(
							transaction.DriveKey,
							transaction.Signer
						));

						const auto& config = pConfigHolder->ConfigAtHeightOrLatest(associatedHeight);
						const auto& pluginConfig = config.Network.GetPluginConfiguration<config::ServiceConfiguration>();

						UnresolvedMosaic storageMosaic{config::GetUnresolvedStorageMosaicId(config.Immutable), pluginConfig.VerificationFee};
						sub.notify(BalanceDebitNotification<1>(transaction.Signer, storageMosaic.MosaicId, storageMosaic.Amount));
						sub.notify(SecretLockNotification<1>(
							transaction.Signer,
							storageMosaic,
							pluginConfig.VerificationDuration,
							LockHashAlgorithm::Op_Internal,
							Hash256(),
							extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, config.Immutable.NetworkIdentifier))));

						break;
					}
					default:
						CATAPULT_LOG(debug) << "invalid version of StartDriveVerificationTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StartDriveVerification, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
