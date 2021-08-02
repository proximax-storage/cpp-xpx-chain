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

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(ReplicatorOnboardingNotification<1>(
					transaction.Signer,
					transaction.Capacity
				));

				const auto signerAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.Signer, config.NetworkIdentifier));
				const auto storageMosaicId = config::GetUnresolvedStorageMosaicId(config);

				// Payments for storage deposit serve as proof of space
				sub.notify(BalanceDebitNotification<1>(
					signerAddress,
					storageMosaicId
				));

				//Payments for streaming deposit serve as proof of space
				const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

				sub.notify(BalanceDebitNotification<1>)(
					signerAddress,
					streamingMosaicId
				));

				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of ReplicatorOnboardingTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(ReplicatorOnboarding, Default, Publish)
}}
