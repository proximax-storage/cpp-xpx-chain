/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SuspendTransactionPlugin.h"
#include "src/model/SuperContractNotifications.h"
#include "src/model/SuspendTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "plugins/txes/multisig/src/model/MultisigNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(SuperContractNotification<1>(transaction.SuperContract, transaction.Type));
				sub.notify(SuspendNotification<1>(
					transaction.Signer,
					transaction.SuperContract
				));
                break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of SuspendTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(Suspend, Default, Publish)
}}
