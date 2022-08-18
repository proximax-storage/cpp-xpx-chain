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
			return [pConfigHolder](const TTransaction &transaction, const PublishContext &context, NotificationSubscriber &sub) {
				auto &blockChainConfig = pConfigHolder->ConfigAtHeightOrLatest(context.AssociatedHeight);
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
						sub.notify(ModifyMultisigNewCosignerNotification<1>(transaction.DriveKey, transaction.Signer));
						// We need to inform user that replicator added to multisig
                        sub.notify(AccountPublicKeyNotification<1>(transaction.DriveKey));

						auto pModification = sub.mempool().malloc(
							CosignatoryModification{model::CosignatoryModificationType::Add, transaction.Signer});
						sub.notify(ModifyMultisigCosignersNotification<1>(transaction.DriveKey, 1, pModification));

						sub.notify(JoinToDriveNotification<1>(
								transaction.DriveKey,
								transaction.Signer
						));

						auto pDeposit = sub.mempool().malloc(model::DriveDeposit(transaction.DriveKey));
						sub.notify(BalanceDebitNotification<1>(
								transaction.Signer,
								UnresolvedMosaicId(blockChainConfig.Immutable.StorageMosaicId.unwrap()),
								UnresolvedAmount(0, UnresolvedAmountType::DriveDeposit, pDeposit)
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
