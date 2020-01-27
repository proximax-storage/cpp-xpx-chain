/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExecuteTransactionPlugin.h"
#include "src/model/SuperContractNotifications.h"
#include "src/model/ExecuteTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "plugins/txes/service/src/model/ServiceNotifications.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber&) {
			switch (transaction.EntityVersion()) {
			case 1: {
			    // TODO: Implement when operation plugin will be done
                break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of ExecuteTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(Execute, Only_Embeddable, Publish)
}}
