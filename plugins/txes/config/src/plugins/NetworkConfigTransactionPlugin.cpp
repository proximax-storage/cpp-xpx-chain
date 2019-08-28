/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "NetworkConfigTransactionPlugin.h"
#include "src/model/NetworkConfigTransaction.h"
#include "src/model/NetworkConfigNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1:
				sub.notify(model::NetworkConfigSignerNotification<1>(transaction.Signer));
				sub.notify(model::NetworkConfigNotification<1>(
					transaction.ApplyHeightDelta,
					transaction.BlockChainConfigSize,
					transaction.BlockChainConfigPtr(),
					transaction.SupportedEntityVersionsSize,
					transaction.SupportedEntityVersionsPtr()));
				break;

			default:
				CATAPULT_LOG(debug) << "invalid version of NetworkConfigTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(NetworkConfig, Default, Publish)
}}
