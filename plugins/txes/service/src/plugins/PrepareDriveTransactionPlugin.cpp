/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PrepareDriveTransactionPlugin.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/PrepareDriveTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(PrepareDriveNotification<1>(
					transaction.Signer,
					transaction.Owner,
					transaction.Duration,
					transaction.BillingPeriod,
					transaction.BillingPrice,
					transaction.DriveSize,
					transaction.Replicas,
					transaction.MinReplicators,
					transaction.PercentApprovers
				));
                break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of PrepareDriveTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(PrepareDrive, Only_Embeddable, Publish)
}}
