/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "LockFundCancelUnlockTransactionPlugin.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/LockFundCancelUnlockTransaction.h"
#include "src/model/LockFundNotifications.h"
using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const PublishContext&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(LockFundCancelUnlockNotification<1>(transaction.Signer, transaction.TargetHeight));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of LockFundCancelUnlockTransaction: " << transaction.EntityVersion();
			}
		}
	}
	DEFINE_TRANSACTION_PLUGIN_FACTORY(LockFundCancelUnlock, Default, Publish)
}}
