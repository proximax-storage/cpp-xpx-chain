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
#include "catapult/model/EntityHasher.h"
#include "src/utils/StorageUtils.h"
#include "sdk/src/extensions/ConversionExtensions.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 2: {
					sub.notify(SignatureNotification<1>(transaction.NodeBootKey, transaction.MessageSignature, transaction.Message));
					sub.notify(ReplicatorNodeBootKeyNotification<1>(transaction.Signer, transaction.NodeBootKey));

					[[fallthrough]];
				}
				case 1: {
					auto hashSeed = CalculateHash(transaction, config.GenerationHash);

					sub.notify(AccountPublicKeyNotification<1>(transaction.Signer));

					const auto storageMosaicId = config::GetUnresolvedStorageMosaicId(config);
					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);
					uint64_t capacity = transaction.Capacity.unwrap();

					//swap xpx to storage unit
					utils::SwapMosaics(
						transaction.Signer,
						{ { storageMosaicId, transaction.Capacity } },
						sub,
						config,
						utils::SwapOperation::Buy
					);

					//swap xpx to streaming unit
					utils::SwapMosaics(
						transaction.Signer,
						{ { streamingMosaicId, Amount(2 * capacity) } },
						sub,
						config,
						utils::SwapOperation::Buy
					);

					sub.notify(ReplicatorOnboardingNotification<1>(transaction.Signer, transaction.Capacity, hashSeed));

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of ReplicatorOnboardingTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(ReplicatorOnboarding, Default, CreatePublisher, config::ImmutableConfiguration)
}}
