/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "DownloadApprovalTransactionPlugin.h"
#include "src/model/DownloadApprovalTransaction.h"
#include "src/utils/StorageUtils.h"
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
					const auto commonDataSize = sizeof(transaction.DownloadChannelId)
												+ sizeof(transaction.SequenceNumber)
												+ sizeof(transaction.ResponseToFinishDownloadTransaction);
					auto* const commonDataPtr = new uint8_t[commonDataSize];
					auto* pCommonData = commonDataPtr;
					utils::WriteToByteArray(pCommonData, transaction.DownloadChannelId);
					utils::WriteToByteArray(pCommonData, transaction.SequenceNumber);
					utils::WriteToByteArray(pCommonData, transaction.ResponseToFinishDownloadTransaction);

					sub.notify(OpinionNotification<1>(
							commonDataSize,
							transaction.OpinionCount,
							transaction.JudgingKeysCount,
							transaction.OverlappingKeysCount,
							transaction.JudgedKeysCount,
							commonDataPtr,
							transaction.PublicKeysPtr(),
							transaction.OpinionIndicesPtr(),
							transaction.BlsSignaturesPtr(),
							transaction.PresentOpinionsPtr(),
							transaction.OpinionsPtr()
					));

				  	sub.notify(DownloadApprovalNotification<1>(
						  	transaction.DownloadChannelId,
							transaction.SequenceNumber
				  	));

					sub.notify(DownloadApprovalPaymentNotification<1>(
							transaction.DownloadChannelId,
							transaction.OpinionCount,
							transaction.JudgingKeysCount,
							transaction.OverlappingKeysCount,
							transaction.JudgedKeysCount,
							transaction.PublicKeysPtr(),
							transaction.OpinionIndicesPtr(),
							transaction.PresentOpinionsPtr(),
							transaction.OpinionsPtr()
					));

					if (transaction.ResponseToFinishDownloadTransaction)
						sub.notify(DownloadChannelRefundNotification<1>(transaction.DownloadChannelId));

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
