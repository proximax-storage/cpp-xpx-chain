/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveDbrbProcessByNetworkTransactionPlugin.h"
#include "src/model/DbrbNotifications.h"
#include "src/model/RemoveDbrbProcessByNetworkTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(RemoveDbrbProcessByNetworkNotification<1>(transaction.ProcessId, transaction.Timestamp, transaction.VotesPtr(), transaction.VoteCount));
				sub.notify(RemoveDbrbProcessNotification<1>(transaction.ProcessId));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of RemoveDbrbProcessByNetworkTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(RemoveDbrbProcessByNetwork, Default, Publish)
}}
