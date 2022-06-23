/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "PlaceSdaExchangeOfferTransactionPlugin.h"
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

                    /* Ensures that the signer has enough account balance to exchange.
                       The offer balance will be returned to the signer upon expiry or after sending a RemoveSdaExchangeOfferTransaction */
                    std::map<UnresolvedMosaicId, Amount> lockAmount = {};
                    auto pSdaOffer = transaction.SdaOffersPtr();
                    for (uint8_t i = 0; i < transaction.SdaOfferCount; ++i, ++pSdaOffer) {
                        auto mosaicId = pSdaOffer->MosaicGive.MosaicId;
                        if (!lockAmount.count(mosaicId)) {
                            lockAmount.emplace(mosaicId, pSdaOffer->MosaicGive.Amount);
                            continue;
                        }
                        lockAmount.at(mosaicId) = Amount(lockAmount.find(mosaicId)->second.unwrap() + pSdaOffer->MosaicGive.Amount.unwrap());
                    }

                    for (auto lock : lockAmount) {
                        if (Amount(0) != lock.second) {
                            sub.notify(BalanceDebitNotification<1>(transaction.Signer, lock.first, lock.second));
                        }
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
