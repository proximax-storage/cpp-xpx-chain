/**
*** Copyright 2020 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "EndFileDownloadTransactionPlugin.h"
#include "catapult/model/Address.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/plugins/PluginUtils.h"
#include "plugins/txes/lock_secret/src/model/SecretLockNotifications.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/config/ServiceConfiguration.h"
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
						sub.notify(AccountPublicKeyNotification<1>(transaction.Recipient));
						sub.notify(BalanceCreditNotification<1>(transaction.Recipient, config::GetUnresolvedReviewMosaicId(config), Amount(transaction.FileCount)));
						auto pFile = transaction.FilesPtr();
						for (auto i = 0u; i < transaction.FileCount; ++i, ++pFile) {
							sub.notify(ProofPublicationNotification<1>(
								transaction.Signer,
								LockHashAlgorithm::Op_Internal,
								utils::CalculateFileDownloadHash(transaction.OperationToken, pFile->FileHash),
								extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.Signer, config.NetworkIdentifier))));
						}

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
