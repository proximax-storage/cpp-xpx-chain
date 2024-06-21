/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "AddOrUpdateDbrbProcessTransactionPlugin.h"
#include "src/model/DbrbNotifications.h"
#include "src/model/AddOrUpdateDbrbProcessTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(AddDbrbProcessNotification<1>(transaction.Signer));
				sub.notify(ActiveHarvestersNotification<3>(transaction.Signer, transaction.HarvesterKeysPtr(), transaction.HarvesterKeysCount, transaction.BlockchainVersion));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of AddOrUpdateDbrbProcessTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(AddOrUpdateDbrbProcess, Default, Publish)
}}
