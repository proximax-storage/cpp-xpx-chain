/**
*** Copyright 2025 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "UpdateDriveSizeTransactionPlugin.h"
#include "catapult/model/StreamingNotifications.h"
#include "src/model/UpdateDriveSizeTransaction.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/NotificationSubscriber.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [&config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(UpdateDriveSizeNotification<1>(
							transaction.DriveKey,
							transaction.NewDriveSize,
							transaction.Signer));

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of UpdateDriveSizeTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(UpdateDriveSize, Default, CreatePublisher, config::ImmutableConfiguration)
}}
