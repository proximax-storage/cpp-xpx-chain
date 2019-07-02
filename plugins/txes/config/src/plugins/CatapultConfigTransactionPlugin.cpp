/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "CatapultConfigTransactionPlugin.h"
#include "src/model/CatapultConfigTransaction.h"
#include "src/model/CatapultConfigNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1:
				sub.notify(model::CatapultConfigSignerNotification<1>(transaction.Signer));
				sub.notify(model::CatapultConfigNotification<1>(
					transaction.ApplyHeightDelta,
					transaction.BlockChainConfigSize,
					transaction.BlockChainConfigPtr(),
					transaction.SupportedEntityVersionsSize,
					transaction.SupportedEntityVersionsPtr()));
				break;

			default:
				CATAPULT_THROW_RUNTIME_ERROR_1("invalid version of CatapultConfigTransaction", transaction.EntityVersion());
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(CatapultConfig, Publish)
}}
