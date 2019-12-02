/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndDriveTransactionPlugin.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/EndDriveTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
				case 1: {
                    sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
                    sub.notify(EndDriveNotification<1>(transaction.DriveKey, transaction.Signer));
					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of EndDriveTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(EndDrive, Default, Publish)
}}
