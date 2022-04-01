/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PlaceSdaExchangeOfferTransactionPlugin.h"
#include "catapult/config/ImmutableConfiguration.h"
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/model/TransactionPluginFactory.h"
#include "src/model/PlaceSdaExchangeOfferTransaction.h"
#include "src/model/SdaExchangeNotifications.h"
#include "sdk/src/extensions/ConversionExtensions.h"

using namespace catapult::model;

namespace catapult { namespace plugins {

	namespace {
		template<typename TTransaction>
		void Publish(const TTransaction& transaction, const Height&, NotificationSubscriber& sub) {
				switch (transaction.EntityVersion()) {
				case 1: {
					sub.notify(PlaceSdaOfferNotification<1>(
							transaction.Signer,
							transaction.SdaOfferCount,
							transaction.SdaOffersPtr()));

					auto pOffer = transaction.SdaOffersPtr();
					for (uint8_t i = 0; i < transaction.SdaOfferCount; ++i, ++pOffer) {
						sub.notify(BalanceDebitNotification<1>(transaction.Signer, pOffer->MosaicGet.MosaicId, pOffer->MosaicGet.Amount));
						sub.notify(BalanceCreditNotification<1>(transaction.Signer, pOffer->MosaicGive.MosaicId, pOffer->MosaicGive.Amount));
					}
					break;
				}
				default:
					CATAPULT_LOG(debug) << "invalid version of PlaceSdaExchangeOfferTransaction: " << transaction.EntityVersion();
			}
		}
	}

	DEFINE_TRANSACTION_PLUGIN_FACTORY(PlaceSdaExchangeOffer, Default, Publish)
}}
