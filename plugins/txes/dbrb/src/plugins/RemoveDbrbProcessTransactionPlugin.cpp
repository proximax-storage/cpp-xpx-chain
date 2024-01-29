/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "RemoveDbrbProcessTransactionPlugin.h"
#include "src/model/DbrbNotifications.h"
#include "src/model/RemoveDbrbProcessTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(RemoveDbrbProcessNotification<1>(transaction.Signer));
				sub.notify(InactiveHarvestersNotification<1>(transaction.HarvesterKeysPtr(), transaction.HarvesterKeysCount));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of RemoveDbrbProcessTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(RemoveDbrbProcess, Default, Publish)
}}
