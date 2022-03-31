/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <catapult/model/Address.h>
#include "src/model/ManualRateChangeTransaction.h"
#include "ManualRateChangeTransactionPlugin.h"
#include "catapult/model/NotificationSubscriber.h"
#include "src/model/InternalLiquidityProviderNotifications.h"
#include "catapult/model/TransactionPluginFactory.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1:
					sub.notify(model::ManualRateChangeNotification<1>(
							transaction.Signer,
							transaction.ProviderMosaicId,
							transaction.CurrencyBalanceIncrease,
							transaction.CurrencyBalanceChange,
							transaction.MosaicBalanceIncrease,
							transaction.MosaicBalanceChange
							));

					break;

				default:
					CATAPULT_LOG(debug) << "invalid version of CreateLiquidityProviderTransaction: "
										<< transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(ManualRateChange, Default, CreatePublisher, config::ImmutableConfiguration);
}}
