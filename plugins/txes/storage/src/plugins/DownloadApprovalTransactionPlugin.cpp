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
												+ sizeof(transaction.ApprovalTrigger);
					auto* const pCommonDataBegin = sub.mempool().malloc<uint8_t>(commonDataSize);
					auto* pCommonData = pCommonDataBegin;
					utils::WriteToByteArray(pCommonData, transaction.DownloadChannelId);
					utils::WriteToByteArray(pCommonData, transaction.ApprovalTrigger);

					sub.notify(OpinionNotification<1>(
							commonDataSize,
							transaction.JudgingKeysCount,
							transaction.OverlappingKeysCount,
							transaction.JudgedKeysCount,
							sizeof(uint64_t),
							pCommonDataBegin,
							transaction.PublicKeysPtr(),
							transaction.SignaturesPtr(),
							transaction.PresentOpinionsPtr(),
							reinterpret_cast<const uint8_t*>(transaction.OpinionsPtr()),
							true
					));

				  	sub.notify(DownloadApprovalNotification<1>(
						  	transaction.DownloadChannelId,
							transaction.ApprovalTrigger,
							transaction.JudgingKeysCount,
							transaction.OverlappingKeysCount,
							transaction.JudgedKeysCount,
							transaction.PublicKeysPtr(),
							transaction.PresentOpinionsPtr()
				  	));

					sub.notify(DownloadApprovalPaymentNotification<1>(
							transaction.DownloadChannelId,
							transaction.JudgingKeysCount,
							transaction.OverlappingKeysCount,
							transaction.JudgedKeysCount,
							transaction.PublicKeysPtr(),
							transaction.PresentOpinionsPtr(),
							transaction.OpinionsPtr()
					));

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
