/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "CatapultUpgradeTransactionPlugin.h"
#include "src/model/CatapultUpgradeTransaction.h"
#include "src/model/CatapultUpgradeNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1:
				sub.notify(model::CatapultUpgradeSignerNotification<1>(transaction.Signer));
				sub.notify(model::CatapultUpgradeVersionNotification<1>(transaction.UpgradePeriod, transaction.NewCatapultVersion));
				break;

			default:
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of CatapultUpgradeTransaction", transaction.EntityVersion());
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(CatapultUpgrade, Publish)
}}
