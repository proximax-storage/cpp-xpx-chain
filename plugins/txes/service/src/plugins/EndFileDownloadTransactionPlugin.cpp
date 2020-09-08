/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndFileDownloadTransactionPlugin.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/EndFileDownloadTransaction.h"
#include "src/utils/ServiceUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction &transaction, const Height&, NotificationSubscriber &sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(DriveNotification<1>(transaction.Signer, transaction.Type));
						sub.notify(AccountPublicKeyNotification<1>(transaction.FileRecipient));
						sub.notify(EndFileDownloadNotification<1>(
							transaction.FileRecipient,
							transaction.OperationToken,
							transaction.FilesPtr(),
							transaction.FileCount
						));
						sub.notify(BalanceCreditNotification<1>(transaction.FileRecipient, config::GetUnresolvedReviewMosaicId(config), Amount(transaction.FileCount)));
						sub.notify(BalanceCreditNotification<1>(
							transaction.Signer,
							config::GetUnresolvedStreamingMosaicId(config),
							utils::CalculateFileDownload(transaction.FilesPtr(), transaction.FileCount)
						));

						break;
					}
					default:
						CATAPULT_LOG(debug) << "invalid version of EndFileDownloadTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(EndFileDownload, Default, CreatePublisher, config::ImmutableConfiguration)
}}
