/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "src/model/MosaicNotifications.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "MosaicRemoveLevyTransactionPlugin.h"
#include "src/model/MosaicRemoveLevyTransaction.h"

using namespace catapult::model;

namespace catapult { namespace plugins {
		
	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
				case 1:
					sub.notify(MosaicRemoveLevyNotification<1>(transaction.MosaicId, transaction.Signer));
					break;

				default:
					CATAPULT_LOG(debug) << "invalid version of MosaicRemoveLevyTransaciton: "
										<< transaction.EntityVersion();
			}
		}
	}
	
	DEFINE_TRANSACTION_PLUGIN_FACTORY(MosaicRemoveLevy, Default, Publish)
}}
