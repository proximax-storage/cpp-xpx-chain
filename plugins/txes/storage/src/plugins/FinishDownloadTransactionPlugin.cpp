/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "FinishDownloadTransactionPlugin.h"
#include "src/model/StorageNotifications.h"
#include "src/model/FinishDownloadTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(FinishDownloadNotification<1>(
						transaction.Signer,
						transaction.DownloadChannelId,
						transaction.FeedbackFeeAmount
				));
				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of FinishDownloadTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(FinishDownload, Default, Publish)
}}
