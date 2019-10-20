/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SellOfferTransactionPlugin.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/SellOfferTransaction.h"
#include "src/model/EmbeddedTransactionHasher.h"
#include "src/model/ExchangeNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					auto transactionHash = ToShortHash(CalculateHash(transaction, config.GenerationHash));
					auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config);

					sub.notify(SellOfferNotification<1>(
						transaction.Signer,
						transactionHash,
						transaction.Deadline,
						transaction.OfferCount,
						transaction.OffersPtr()));
					sub.notify(MatchedSellOfferNotification<1>(
						transaction.Signer,
						transactionHash,
						transaction.MatchedOfferCount,
						transaction.MatchedOffersPtr(),
						currencyMosaicId,
						sub));

					auto pOffer = transaction.OffersPtr();
					for (uint8_t i = 0; i < transaction.OfferCount; ++i, ++pOffer) {
						sub.notify(BalanceDebitNotification<1>(transaction.Signer, pOffer->Mosaic.MosaicId, pOffer->Mosaic.Amount));
					}

					auto pMatchedOffer = transaction.MatchedOffersPtr();
					for (uint8_t i = 0; i < transaction.MatchedOfferCount; ++i, ++pMatchedOffer) {
						sub.notify(BalanceCreditNotification<1>(transaction.Signer, currencyMosaicId, pMatchedOffer->Cost));
						sub.notify(BalanceCreditNotification<1>(pMatchedOffer->TransactionSigner, pMatchedOffer->Mosaic.MosaicId, pMatchedOffer->Mosaic.Amount));
					}
					break;
				}
				default:
					CATAPULT_LOG(debug) << "invalid version of SellOfferTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(SellOffer, Default, CreatePublisher, config::ImmutableConfiguration)
}}
