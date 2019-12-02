/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "JoinToDriveTransactionPlugin.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/JoinToDriveTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "sdk/src/extensions/ConversionExtensions.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder> &pConfigHolder) {
			return [pConfigHolder](const TTransaction &transaction, const Height &associatedHeight,
								   NotificationSubscriber &sub) {
				auto &blockChainConfig = pConfigHolder->ConfigAtHeightOrLatest(associatedHeight);
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
						sub.notify(ModifyMultisigNewCosignerNotification<1>(transaction.DriveKey, transaction.Signer));
						// We need to inform user that replicator added to multisig
                        sub.notify(AccountPublicKeyNotification<1>(transaction.DriveKey));

						// TODO: Fix memory leak
						auto modification = new CosignatoryModification{model::CosignatoryModificationType::Add,
																		transaction.Signer};
						sub.notify(ModifyMultisigCosignersNotification<1>(transaction.DriveKey, 1, modification));

						sub.notify(JoinToDriveNotification<1>(
								transaction.DriveKey,
								transaction.Signer
						));

						// TODO: Fix memory leak
						auto deposit = new model::DriveDeposit{ transaction.DriveKey };
						sub.notify(BalanceTransferNotification<1>(
								transaction.Signer,
								extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, blockChainConfig.Immutable.NetworkIdentifier)),
								UnresolvedMosaicId(blockChainConfig.Immutable.StorageMosaicId.unwrap()),
								UnresolvedAmount(0, UnresolvedAmountType::DriveDeposit, reinterpret_cast<uint8_t*>(deposit))
							)
						);
						break;
					}

					default:
						CATAPULT_LOG(debug) << "invalid version of JoinToDriveTransaction: "
											<< transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(JoinToDrive, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
