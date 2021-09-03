/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorOnboardingTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/ReplicatorOnboardingTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/utils/StorageUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(DriveNotification<1>(transaction.Signer, transaction.Type));
					sub.notify(ReplicatorOnboardingNotification<1>(transaction.Signer, transaction.BlsKey, transaction.Capacity));

					const auto signerAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.Signer, config.NetworkIdentifier));
					const auto storageMosaicId = config::GetUnresolvedStorageMosaicId(config);
					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

					//swap xpx to storage unit
					utils::SwapMosaics(
						transaction.Signer,
						{ { storageMosaicId, Amount(transaction.Capacity) } },
						sub,
						config,
						utils::SwapOperation::Buy
					);

					//swap xpx to streaming unit
					utils::SwapMosaics(
						transaction.Signer,
						{ { streamingMosaicId, Amount(2 * transaction.Capacity) } },
						sub,
						config,
						utils::SwapOperation::Buy
					);

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of ReplicatorOnboardingTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(ReplicatorOnboarding, Default, CreatePublisher)
}}
