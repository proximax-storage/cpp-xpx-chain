/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "BuyOfferTransactionPlugin.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/model/EntityHasher.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "sdk/src/extensions/ConversionExtensions.h"
#include "src/model/BuyOfferTransaction.h"
#include "src/model/EmbeddedTransactionHasher.h"
#include "src/model/ExchangeNotifications.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		auto CreatePublisher(const config::ImmutableConfiguration& config) {
			return [config](const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				if (1 == transaction.EntityVersion()) {
					auto transactionHash = ToShortHash(CalculateHash(transaction, config.GenerationHash));
					auto currencyMosaicId = extensions::CastToUnresolvedMosaicId(config.CurrencyMosaicId);

					sub.notify(BuyOfferNotification<1>(
						transaction.Signer,
						transactionHash,
						transaction.Deadline,
						transaction.OfferCount,
						transaction.OffersPtr()));
					sub.notify(MatchedBuyOfferNotification<1>(
						transaction.Signer,
						transactionHash,
						transaction.MatchedOfferCount,
						transaction.MatchedOffersPtr(),
						currencyMosaicId,
						sub));

					Amount lockAmount(0);
					auto pOffer = transaction.OffersPtr();
					for (uint8_t i = 0; i < transaction.OfferCount; ++i, ++pOffer) {
						lockAmount = Amount(lockAmount.unwrap() + pOffer->Cost.unwrap());
					}
					sub.notify(BalanceDebitNotification<1>(transaction.Signer, currencyMosaicId, lockAmount));

					auto pMatchedOffer = transaction.MatchedOffersPtr();
					for (uint8_t i = 0; i < transaction.MatchedOfferCount; ++i, ++pMatchedOffer) {
						sub.notify(BalanceCreditNotification<1>(transaction.Signer, pMatchedOffer->Mosaic.MosaicId, pMatchedOffer->Mosaic.Amount));
						sub.notify(BalanceCreditNotification<1>(pMatchedOffer->TransactionSigner, currencyMosaicId, pMatchedOffer->Cost));
					}

				} else {
						CATAPULT_LOG(debug) << "invalid version of BuyOfferTransaction: " << transaction.EntityVersion();
				}
			};
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY_WITH_CONFIG(BuyOffer, Default, CreatePublisher, config::ImmutableConfiguration)
}}
