/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "ExchangeTransactionPlugin.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/ExchangeTransaction.h"
#include "src/model/ExchangeNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);
					sub.notify(ExchangeNotification<1>(
						transaction.Signer,
						transaction.OfferCount,
						transaction.OffersPtr()));

					auto pOffer = transaction.OffersPtr();
					for (uint8_t i = 0; i < transaction.OfferCount; ++i, ++pOffer) {
						if (model::OfferType::Sell == pOffer->Type) {
							sub.notify(BalanceDebitNotification<1>(transaction.Signer, currencyMosaicId, pOffer->Cost));
						} else {
							sub.notify(BalanceDebitNotification<1>(transaction.Signer, pOffer->Mosaic.MosaicId, pOffer->Mosaic.Amount));
						}
						sub.notify(BalanceCreditNotification<1>(
							(model::OfferType::Sell == pOffer->Type) ? transaction.Signer : pOffer->Owner,
							pOffer->Mosaic.MosaicId, pOffer->Mosaic.Amount));
						sub.notify(BalanceCreditNotification<1>(
							(model::OfferType::Sell == pOffer->Type) ? pOffer->Owner : transaction.Signer,
							currencyMosaicId, pOffer->Cost));
					}
					break;
				}
				default:
					CATAPULT_LOG(debug) << "invalid version of ExchangeTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(Exchange, Default, CreatePublisher, config::ImmutableConfiguration)
}}
