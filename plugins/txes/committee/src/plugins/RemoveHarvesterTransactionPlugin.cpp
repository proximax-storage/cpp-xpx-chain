/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "RemoveHarvesterTransactionPlugin.h"
#include "src/model/RemoveHarvesterTransaction.h"
#include "src/model/CommitteeNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1:
				sub.notify(model::RemoveHarvesterNotification<1>(transaction.Signer));
				break;

			default:
				CATAPULT_LOG(debug) << "invalid version of RemoveHarvesterTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(RemoveHarvester, Default, Publish)
}}
