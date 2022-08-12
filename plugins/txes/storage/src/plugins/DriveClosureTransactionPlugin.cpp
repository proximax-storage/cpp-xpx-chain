/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DriveClosureTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/DriveClosureTransaction.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [&config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					auto transactionHash = CalculateHash(transaction, config.GenerationHash);
					sub.notify(DriveClosureNotification<1>(
							transactionHash,
							transaction.DriveKey,
							transaction.Signer));

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of DriveClosureTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(DriveClosure, Default, CreatePublisher, config::ImmutableConfiguration)
}}
