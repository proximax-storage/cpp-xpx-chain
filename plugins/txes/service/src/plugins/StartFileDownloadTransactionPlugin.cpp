/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "StartFileDownloadTransactionPlugin.h"
#include "catapult/model/Address.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/plugins/PluginUtils.h"
#include "plugins/txes/lock_secret/src/model/SecretLockNotifications.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/config/ServiceConfiguration.h"
#include "src/model/ServiceNotifications.h"
#include "src/model/StartFileDownloadTransaction.h"
#include "src/utils/ServiceUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
			return [pConfigHolder](const TTransaction &transaction, const Height& associatedHeight, NotificationSubscriber &sub) {
				switch (transaction.EntityVersion()) {
					case 1: {
						sub.notify(DriveNotification<1>(transaction.DriveKey, transaction.Type));
						sub.notify(StartFileDownloadNotification<1>(
							transaction.DriveKey,
							transaction.Signer,
							transaction.FilesPtr(),
							transaction.FileCount
						));

						const auto& config = pConfigHolder->ConfigAtHeightOrLatest(associatedHeight);
						const auto& pluginConfig = config.Network.GetPluginConfiguration<config::ServiceConfiguration>();
						auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config.Immutable);
						Amount totalAmount;
						auto pFile = transaction.FilesPtr();
						for (auto i = 0u; i < transaction.FileCount; ++i, ++pFile) {
							auto amount = utils::CalculateFileDownload(pFile->FileSize);
							totalAmount = totalAmount + amount;
							sub.notify(SecretLockNotification<1>(
								transaction.Signer,
								UnresolvedMosaic{ streamingMosaicId, amount },
								pluginConfig.DownloadDuration,
								LockHashAlgorithm::Op_Internal,
								utils::CalculateFileDownloadHash(transaction.Signer, transaction.DriveKey, pFile->FileHash),
								extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.DriveKey, config.Immutable.NetworkIdentifier))));
						}
						sub.notify(BalanceDebitNotification<1>(transaction.Signer, streamingMosaicId, totalAmount));

						break;
					}
					default:
						CATAPULT_LOG(debug) << "invalid version of StartFileDownloadTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(StartFileDownload, Default, CreatePublisher, std::shared_ptr<config::BlockchainConfigurationHolder>)
}}
