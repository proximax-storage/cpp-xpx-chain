/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadApprovalTransactionPlugin.h"
#include "src/model/DownloadApprovalTransaction.h"
#include "catapult/model/Address.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/StorageNotifications.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction &transaction, const Height &associatedHeight, NotificationSubscriber &sub) {
			  	switch (transaction.EntityVersion()) {
			  	case 1: {
				  	sub.notify(DownloadApprovalNotification<1>(
						  	transaction.DownloadChannelId,
							transaction.SequenceNumber,
						  	transaction.ResponseToFinishDownloadTransaction,
						  	transaction.OpinionCount,
							transaction.JudgingCount,
							transaction.JudgedCount,
							transaction.OpinionElementCount,
							transaction.PublicKeysPtr(),
							transaction.OpinionIndicesPtr(),
							transaction.BlsSignaturesPtr(),
							transaction.PresentOpinionsPtr(),
							transaction.OpinionsPtr()
				  	));
				  	break;
			  	}

			  	default:
				  	CATAPULT_LOG(debug) << "invalid version of DownloadApprovalTransaction: " << transaction.EntityVersion();
			  	}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(DownloadApproval, Default, CreatePublisher, config::ImmutableConfiguration)
}}
