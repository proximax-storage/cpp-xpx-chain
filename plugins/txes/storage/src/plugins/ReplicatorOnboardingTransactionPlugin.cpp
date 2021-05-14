/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorOnboardingTransactionPlugin.h"
#include "src/model/StorageNotifications.h"
#include "src/model/ReplicatorOnboardingTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(ReplicatorOnboardingNotification<1>(
						transaction.Signer,
						transaction.Capacity
				));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of ReplicatorOnboardingTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(ReplicatorOnboarding, Default, Publish)
}}
