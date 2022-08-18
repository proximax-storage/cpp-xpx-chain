/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "UploadFileTransactionPlugin.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/SuperContractNotifications.h"
#include "src/model/UploadFileTransaction.h"
#include "plugins/txes/service/src/model/ServiceNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const PublishContext&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(SuperContractNotification<1>(transaction.Signer, transaction.Type));
					    sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
						sub.notify(DriveFileSystemNotification<1>(
							transaction.DriveKey,
							transaction.Signer,
							transaction.RootHash,
							transaction.XorRootHash,
							transaction.AddActionsCount,
							transaction.AddActionsPtr(),
							transaction.RemoveActionsCount,
							transaction.RemoveActionsPtr()
						));

						break;
					}
					default:
						CATAPULT_LOG(debug) << "invalid version of UploadFileTransaction: " << transaction.EntityVersion();
				}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(UploadFile, Only_Embeddable, Publish)
}}
