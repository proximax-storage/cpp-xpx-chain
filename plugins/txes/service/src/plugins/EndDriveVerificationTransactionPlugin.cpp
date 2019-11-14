/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndDriveVerificationTransactionPlugin.h"
#include "catapult/model/Address.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/plugins/PluginUtils.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/config/ServiceConfiguration.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/EndDriveVerificationTransaction.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder> &pConfigHolder) {
			return [pConfigHolder](const TTransaction &transaction, const Height &associatedHeight, NotificationSubscriber &sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						const auto& config = pConfigHolder->ConfigAtHeightOrLatest(associatedHeight);
						auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config.Immutable);
						auto networkIdentifier = config.Immutable.NetworkIdentifier;
						const auto& pluginConfig = config.Network.GetPluginConfiguration<config::ServiceConfiguration>(PLUGIN_NAME_HASH(service));
						auto pFailure = transaction.FailuresPtr();
						for (auto i = 0u; i < transaction.FailureCount; ++i, ++pFailure) {
							// TODO: Fix memory leak
							auto modification = new CosignatoryModification{model::CosignatoryModificationType::Del, pFailure->Replicator};
							sub.notify(ModifyMultisigCosignersNotification<1>(transaction.Signer, 1, modification));

							if (Amount(0) != pluginConfig.FailedVerificationPayment) {
								auto replicatorAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(pFailure->Replicator, networkIdentifier));
								sub.notify(BalanceTransferNotification<1>(
									transaction.Signer,
									replicatorAddress,
									streamingMosaicId,
									pluginConfig.FailedVerificationPayment));
							}
						}

						/// EndDriveVerificationNotification should be published after drive multisig modifications.
						sub.notify(EndDriveVerificationNotification<1>(transaction.Signer));

						break;
					}
					default:
						CATAPULT_LOG(debug) << "invalid version of EndDriveVerificationTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(EndDriveVerification, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
