/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StartFileDownloadTransactionPlugin.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/StartFileDownloadTransaction.h"
#include "src/utils/ServiceUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction &transaction, const Height&, NotificationSubscriber &sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
						auto operationToken = model::CalculateHash(transaction, config.GenerationHash);
						sub.notify(StartFileDownloadNotification<1>(
							transaction.DriveKey,
							transaction.Signer,
							operationToken,
							transaction.FilesPtr(),
							transaction.FileCount
						));
						sub.notify(BalanceDebitNotification<1>(
							transaction.Signer,
							config::GetUnresolvedStreamingMosaicId(config),
							utils::CalculateFileDownload(transaction.FilesPtr(), transaction.FileCount)
						));

						break;
					}
					default:
						CATAPULT_LOG(debug) << "invalid version of StartFileDownloadTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StartFileDownload, Default, CreatePublisher, config::ImmutableConfiguration)
}}
