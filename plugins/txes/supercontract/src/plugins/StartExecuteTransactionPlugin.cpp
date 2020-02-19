/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/EntityHasher.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "StartExecuteTransactionPlugin.h"
#include "plugins/txes/operation/src/config/OperationConfiguration.h"
#include "plugins/txes/operation/src/model/OperationNotifications.h"
#include "src/model/StartExecuteTransaction.h"
#include "src/model/SuperContractNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder>& configHolder) {
			return [configHolder](const TTransaction &transaction, const Height& associatedHeight, NotificationSubscriber &sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						const auto& config = configHolder->Config(associatedHeight);
						auto operationDuration = config.Network.GetPluginConfiguration<config::OperationConfiguration>().MaxOperationDuration;
						sub.notify(OperationMosaicNotification<1>(transaction.MosaicsPtr(), transaction.MosaicsCount));
						auto operationToken = model::CalculateHash(transaction, config.Immutable.GenerationHash);
						sub.notify(AccountPublicKeyNotification<1>(transaction.SuperContract));
						sub.notify(SuperContractNotification<1>(transaction.SuperContract));
						sub.notify(StartOperationNotification<1>(
							operationToken,
							transaction.Signer,
							&transaction.SuperContract,
							1,
							transaction.MosaicsPtr(),
							transaction.MosaicsCount,
							operationDuration.blocks(config.Network.BlockGenerationTargetTime)
						));
						auto pMosaic = transaction.MosaicsPtr();
						for (auto i = 0u; i < transaction.MosaicsCount; ++i, ++pMosaic) {
							sub.notify(BalanceDebitNotification<1>(transaction.Signer, pMosaic->MosaicId, pMosaic->Amount));
						}
						break;
					}

					default:
						CATAPULT_LOG(debug) << "invalid version of StartExecuteTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StartExecute, Only_Embeddable, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
