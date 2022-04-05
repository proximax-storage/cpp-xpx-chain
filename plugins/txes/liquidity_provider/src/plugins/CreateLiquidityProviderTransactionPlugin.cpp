/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/model/Address.h>
#include "src/model/CreateLiquidityProviderTransaction.h"
#include "CreateLiquidityProviderTransactionPlugin.h"
#include "catapult/model/NotificationSubscriber.h"
#include "src/model/InternalLiquidityProviderNotifications.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "catapult/model/EntityHasher.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					auto providerKey = Key(CalculateHash(transaction, config.GenerationHash).array());

					sub.notify(AccountPublicKeyNotification<1>(providerKey));

					sub.notify(AccountPublicKeyNotification<1>(transaction.SlashingAccount));

					const auto providerAddress = extensions::CopyToUnresolvedAddress(
							PublicKeyToAddress(providerKey, config.NetworkIdentifier));
					const auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);

					sub.notify(BalanceTransferNotification<1>(
							transaction.Signer, providerAddress, currencyMosaicId, transaction.CurrencyDeposit));

					sub.notify(BalanceCreditNotification<1>(providerKey, transaction.ProviderMosaicId, transaction.InitialMosaicsMinting));

					sub.notify(CreateLiquidityProviderNotification<1>(
							providerKey,
							transaction.Signer,
							transaction.ProviderMosaicId,
							transaction.CurrencyDeposit,
							transaction.InitialMosaicsMinting,
							transaction.SlashingPeriod,
							transaction.WindowSize,
							transaction.SlashingAccount,
							transaction.Alpha,
							transaction.Beta));

					break;
				}

				default:
					CATAPULT_LOG(debug) << "invalid version of CreateLiquidityProviderTransaction: "
										<< transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(CreateLiquidityProvider, Default, CreatePublisher, config::ImmutableConfiguration)
}}
