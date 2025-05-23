/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "BlockchainUpgradeTransactionPlugin.h"
#include "src/model/BlockchainUpgradeTransaction.h"
#include "src/model/BlockchainUpgradeNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1:
				sub.notify(model::BlockchainUpgradeSignerNotification<1>(transaction.Signer));
				sub.notify(model::BlockchainUpgradeVersionNotification<1>(transaction.UpgradePeriod, transaction.NewBlockchainVersion));
				break;

			default:
				CATAPULT_LOG(debug) << "invalid version of BlockchainUpgradeTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(BlockchainUpgrade, Default, Publish)
}}
