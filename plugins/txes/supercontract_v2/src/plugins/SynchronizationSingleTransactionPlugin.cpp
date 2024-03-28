/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/crypto/Hashes.h>
#include "SynchronizationSingleTransactionPlugin.h"
#include "src/model/SynchronizationSingleTransaction.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/NotificationSubscriber.h"
#include "src/model/InternalSuperContractNotifications.h"
#include "catapult/model/SupercontractNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {

		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(model::ContractStateUpdateNotification<1>(transaction.ContractKey));
					sub.notify(model::SynchronizationSingleNotification<1>(
							transaction.ContractKey, transaction.BatchId, transaction.Signer));

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of SynchronizationSingleTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(SynchronizationSingle, Default, CreatePublisher, config::ImmutableConfiguration)
}}
