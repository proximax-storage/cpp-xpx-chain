/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/crypto/Hashes.h>
#include "AutomaticExecutionsPaymentTransactionPlugin.h"
#include "catapult/model/SupercontractNotifications.h"
#include "src/model/AutomaticExecutionsPaymentTransaction.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "catapult/model/NotificationSubscriber.h"
#include "src/utils/SwapUtils.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					auto contractKey = transaction.ContractKey;
					sub.notify(AccountPublicKeyNotification<1>(contractKey));

					Hash256 paymentHash;
					crypto::Sha3_256_Builder sha3;
					sha3.update({contractKey, config.GenerationHash});
					sha3.final(paymentHash);
					auto contractExecutionPaymentKey = Key(paymentHash.array());
					sub.notify(AccountPublicKeyNotification<1>(contractExecutionPaymentKey));

					sub.notify(AutomaticExecutionsReplenishmentNotification<1>(
							contractKey, transaction.AutomaticExecutionsNumber));

					const auto streamingMosaicId = config::GetUnresolvedStreamingMosaicId(config);
					const auto scMosaicId = config::GetUnresolvedSuperContractMosaicId(config);

					const auto pAutomaticExecutionWork = sub.mempool().malloc(
							model::AutomaticExecutorWork(contractKey, transaction.AutomaticExecutionsNumber));
					utils::SwapMosaics(
							transaction.Signer,
							contractExecutionPaymentKey,
							{ std::make_pair(
									scMosaicId,
									UnresolvedAmount(
											0,
											UnresolvedAmountType::AutomaticExecutionWork,
											pAutomaticExecutionWork)) },
							sub,
							config,
							utils::SwapOperation::Buy);

					const auto pAutomaticDownloadWork = sub.mempool().malloc(
							model::AutomaticExecutorWork(contractKey, transaction.AutomaticExecutionsNumber));
					utils::SwapMosaics(
							transaction.Signer,
							contractExecutionPaymentKey,
							{ std::make_pair(
									streamingMosaicId,
									UnresolvedAmount(
											0, UnresolvedAmountType::AutomaticExecutionWork, pAutomaticDownloadWork)) },
							sub,
							config,
							utils::SwapOperation::Buy);

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of DeployContractTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(AutomaticExecutionsPayment, Default, CreatePublisher, config::ImmutableConfiguration)
}}
