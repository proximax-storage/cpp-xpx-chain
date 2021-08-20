/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ReplicatorOffboardingTransactionPlugin.h"
#include "catapult/model/StorageNotifications.h"
#include "src/model/ReplicatorOffboardingTransaction.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
			switch (transaction.EntityVersion()) {
			case 1: {
				sub.notify(ReplicatorOffboardingNotification<1>(
					transaction.Signer,
				));

				const auto signerAddress = extensions::CopyToUnresolvedAddress(PublicKeyToAddress(transaction.Signer, config.NetworkIdentifier));
				const auto storageMosaicId = config::GetUnresolvedStorageMosaicId(config);
				const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);

				//swap storage unit to xpx
				SwapMosaics(
					transaction.Signer, 
					{ model::UnresolvedMosaic{ storageMosaicId, Amount(signerState) } },
					sub,
					config.Immutable,
					Sell
				);

				// Payments for Storage Deposit Returning to signer
				sub.notify(BalanceCreditNotification<1>(
					signerAddress,
					storageMosaicId,
					Amount(signerState)
				));

				//swap streaming unit to xpx
				auto streamingUnitXPX = SwapMosaics(
					transaction.Signer, 
					{ model::UnresolvedMosaic{ streamingMosaicId, Amount() } },
					sub,
					config.Immutable,
					Sell
				);

				//Payments for streaming deposit return (streaming deposit minus streaming deposit slashing)
				sub.notify(BalanceCreditNotification<1>)(
					signerAddress,
					streamingMosaicId,
					streamingUnitXPX
				));

				//Transfer for streamming deposit slashing 
				sub.notify(BalanceCreditNotification<1>)(
					signerAddress,
					streamingMosaicId
				));

				break;
			}

			default:
				CATAPULT_LOG(debug) << "invalid version of ReplicatorOffboardingTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(ReplicatorOffboarding, Default, Publish)
}}
