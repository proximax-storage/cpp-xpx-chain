/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

// TODO: Check includes (commented out are present in DownloadTransactionPlugin.cpp, but not in DataModificationTransactionPlugin.cpp)
//#include "tools/tools/ToolKeys.h"
//#include "sdk/src/extensions/ConversionExtensions.h"
#include "PrepareDriveTransactionPlugin.h"
#include "src/model/StorageNotifications.h"
#include "src/model/PrepareDriveTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/EntityHasher.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

		namespace {
			template<typename TTransaction>
			auto CreatePublisher(const config::ImmutableConfiguration& config) {
				return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
					switch (transaction.EntityVersion()) {
					case 1: {
						auto driveHash = CalculateHash(transaction, config.GenerationHash);
						sub.notify(PrepareDriveNotification<1>(
								transaction.Signer,
								Key(driveHash.array()),	// TODO: Check driveKey generation
								transaction.DriveSize,
								transaction.ReplicatorCount));
						break;
					}

					default:
						CATAPULT_LOG(debug) << "invalid version of PrepareDriveTransaction: " << transaction.EntityVersion();
					}
				};
			}
		}

		DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(PrepareDrive, Default, CreatePublisher, config::ImmutableConfiguration)
	}}
