/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StartExecuteTransactionPlugin.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "plugins/txes/operation/src/config/OperationConfiguration.h"
#include "plugins/txes/operation/src/plugins/TransactionPublishers.h"
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
						sub.notify(AccountPublicKeyNotification<1>(transaction.SuperContract));
						sub.notify(SuperContractNotification<1>(transaction.SuperContract, transaction.Type));
						sub.notify(StartExecuteNotification<1>(transaction.SuperContract));
						break;
					}

					default:
						CATAPULT_LOG(debug) << "invalid version of StartExecuteTransaction: " << transaction.EntityVersion();
				}

				const auto& config = configHolder->Config(associatedHeight);
				auto operationDuration = config.Network.GetPluginConfiguration<config::OperationConfiguration>().MaxOperationDuration;
				StartOperationPublisher(transaction, sub, config.Immutable.GenerationHash, "StartExecuteTransaction", &transaction.SuperContract, 1,
					operationDuration.blocks(config.Network.BlockGenerationTargetTime));
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StartExecute, Only_Embeddable, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
