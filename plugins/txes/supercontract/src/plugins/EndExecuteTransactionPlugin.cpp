/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "EndExecuteTransactionPlugin.h"
#include "plugins/txes/operation/src/model/OperationNotifications.h"
#include "src/model/EndExecuteTransaction.h"
#include "src/model/SuperContractNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(OperationMosaicNotification<1>(transaction.MosaicsPtr(), transaction.MosaicCount));
					sub.notify(EndOperationNotification<1>(
						transaction.Signer,
						transaction.OperationToken,
						transaction.MosaicsPtr(),
						transaction.MosaicCount,
						transaction.Result
					));

					break;
				}
				default:
					CATAPULT_LOG(debug) << "invalid version of EndExecuteTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(EndExecute, Only_Embeddable, Publish)
}}
