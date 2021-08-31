/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "tools/tools/ToolKeys.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "DataModificationSingleApprovalTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/DataModificationSingleApprovalTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(DataModificationSingleApprovalNotification<1>(
							transaction.Signer,
							transaction.DriveKey,
							transaction.DataModificationId,
							transaction.UploadOpinionPairCount,
							transaction.UploaderKeysPtr(),
							transaction.UploadOpinionPtr(),
							transaction.UsedDriveSize
					));

					const auto signerAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.Signer, config.NetworkIdentifier));
					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

					// Payments for Download Work to the signer Replicator
					const auto pDownloadWork = sub.mempool().malloc(model::DownloadWork(transaction.DriveKey, transaction.Signer));
					sub.notify(BalanceTransferNotification<1>(
							transaction.DriveKey,
							signerAddress,
							streamingMosaicId,
							UnresolvedAmount(0, UnresolvedAmountType::DownloadWork, pDownloadWork)
					));

					// Payments for Upload Work to the Drive Owner and other Replicators of the Drive
					auto uploaderKeysPtr = transaction.UploaderKeysPtr();
					auto uploadOpinionPtr = transaction.UploadOpinionPtr();

					for (auto i = 0u; i < transaction.UploadOpinionPairCount; ++i, ++uploaderKeysPtr, ++uploadOpinionPtr) {
						const auto pUploadWork = sub.mempool().malloc(model::UploadWork(transaction.DriveKey, transaction.Signer, *uploadOpinionPtr));
						const auto uploaderAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(*uploaderKeysPtr, config.NetworkIdentifier));
						sub.notify(BalanceTransferNotification<1>(
								transaction.DriveKey,
								uploaderAddress,
								streamingMosaicId,
								UnresolvedAmount(0, UnresolvedAmountType::UploadWork, pUploadWork)
						));
					}

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of DataModificationSingleApprovalTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(DataModificationSingleApproval, Default, CreatePublisher, config::ImmutableConfiguration)
}}
