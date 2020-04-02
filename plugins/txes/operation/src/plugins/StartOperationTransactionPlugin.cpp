/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StartOperationTransactionPlugin.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/StartOperationTransaction.h"
#include "TransactionPublishers.h"

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction &transaction, const Height&, NotificationSubscriber &sub) {
				StartOperationPublisher(transaction, sub, config.GenerationHash, "StartOperationTransaction",
					transaction.ExecutorsPtr(), transaction.ExecutorCount, transaction.Duration);

				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(OperationDurationNotification<1>(transaction.Duration));
						break;
					}

					default:
						CATAPULT_LOG(debug) << "invalid version of StartExecuteTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StartOperation, Default, CreatePublisher, config::ImmutableConfiguration)
}}
