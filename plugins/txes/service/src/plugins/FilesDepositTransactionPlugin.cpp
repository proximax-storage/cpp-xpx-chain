/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/Address.h"
#include "FilesDepositTransactionPlugin.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/FilesDepositTransaction.h"
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
						sub.notify(FilesDepositNotification<1>(
							transaction.DriveKey,
							transaction.Signer,
							transaction.FilesCount,
							transaction.FilesPtr()
						));
						sub.notify(AccountPublicKeyNotification<1>(transaction.DriveKey));

						auto filesPtr = transaction.FilesPtr();
						auto streamingMosaicId = UnresolvedMosaicId(blockChainConfig.Immutable.StreamingMosaicId.unwrap());

						for (auto i = 0u; i < transaction.FilesCount; ++i, ++filesPtr) {
							auto pDeposit = sub.mempool().malloc(model::FileDeposit(transaction.DriveKey, filesPtr->FileHash));
							sub.notify(BalanceDebitNotification<1>(
								transaction.Signer,
								streamingMosaicId,
								UnresolvedAmount(0, UnresolvedAmountType::FileDeposit, pDeposit)
							));
						}

						break;
					}

					default:
						CATAPULT_LOG(debug) << "invalid version of FilesDepositTransaction: "
											<< transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(FilesDeposit, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
