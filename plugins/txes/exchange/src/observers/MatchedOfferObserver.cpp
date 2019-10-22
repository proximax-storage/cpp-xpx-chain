/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/OfferCache.h"

namespace catapult { namespace observers {

	namespace {
		void UpdateOfferExpiry(cache::OfferCacheDelta& cache, state::OfferEntry& entry, const Height& height) {
			auto expiryHeight = entry.fulfilled() ? height : entry.deadline();
			cache.updateExpiryHeight(entry.transactionHash(), entry.expiryHeight(), expiryHeight);
			entry.setExpiryHeight(expiryHeight);
		}
	}

	DEFINE_OBSERVER(MatchedOffer, model::MatchedOfferNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& offerCache = context.Cache.sub<cache::OfferCache>();
		auto& suggestedOfferEntry = offerCache.find(notification.TransactionHash).get();

		auto pMatchedOffer = notification.MatchedOffersPtr;
		for (uint8_t i = 0; i < notification.MatchedOfferCount; ++i, ++pMatchedOffer) {
			const auto& mosaicId = pMatchedOffer->Mosaic.MosaicId;
			auto& acceptedOfferEntry = offerCache.find(pMatchedOffer->TransactionHash).get();

			if (NotifyMode::Commit == context.Mode) {
				suggestedOfferEntry.offers().at(mosaicId) -= *pMatchedOffer;
				acceptedOfferEntry.offers().at(mosaicId) -= *pMatchedOffer;
			} else {
				suggestedOfferEntry.offers().at(mosaicId) += *pMatchedOffer;
				acceptedOfferEntry.offers().at(mosaicId) += *pMatchedOffer;
			}

			UpdateOfferExpiry(offerCache, acceptedOfferEntry, context.Height);
		}

		UpdateOfferExpiry(offerCache, suggestedOfferEntry, context.Height);
	});
}}
