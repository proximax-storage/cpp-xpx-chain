/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndExecuteTransactionPlugin.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/EndExecuteTransaction.h"
#include "src/model/SuperContractNotifications.h"
#include "plugins/txes/operation/src/plugins/TransactionPublishers.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const PublishContext&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(SuperContractNotification<1>(transaction.Signer, transaction.Type));
					sub.notify(EndExecuteNotification<1>(transaction.Signer));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of EndExecuteTransaction: " << transaction.EntityVersion();
			}

			EndOperationPublisher(transaction, sub, "EndExecuteTransaction");
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(EndExecute, Only_Embeddable, Publish)
}}
