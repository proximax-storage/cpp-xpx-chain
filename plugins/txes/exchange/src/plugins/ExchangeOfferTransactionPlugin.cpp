/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeOfferTransactionPlugin.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/ExchangeOfferTransaction.h"
#include "src/model/ExchangeNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction, VersionType version>
		void Publish(const TTransaction& transaction, NotificationSubscriber& sub, const config::ImmutableConfiguration& config) {
			sub.notify(OfferNotification<version>(
				transaction.Signer,
				transaction.OfferCount,
				transaction.OffersPtr()));

			Amount lockAmount(0);
			auto pOffer = transaction.OffersPtr();
			for (uint8_t i = 0; i < transaction.OfferCount; ++i, ++pOffer) {
				if (model::OfferType::Buy == pOffer->Type) {
					lockAmount = Amount(lockAmount.unwrap() + pOffer->Cost.unwrap());
				} else {
					sub.notify(BalanceDebitNotification<1>(transaction.Signer, pOffer->Mosaic.MosaicId, pOffer->Mosaic.Amount));
				}
			}

			if (Amount(0) != lockAmount) {
				auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);
				sub.notify(BalanceDebitNotification<1>(transaction.Signer, currencyMosaicId, lockAmount));
			}
		}

		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					Publish<TTransaction, 1>(transaction, sub, config);
					break;
				}
				case 2: {
					Publish<TTransaction, 2>(transaction, sub, config);
					break;
				}
				default:
					CATAPULT_LOG(debug) << "invalid version of ExchangeOfferTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(ExchangeOffer, Default, CreatePublisher, config::ImmutableConfiguration)
}}
