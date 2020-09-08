/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	namespace {
		template<typename TOfferMap>
		state::OfferBase& ModifyOffer(TOfferMap& offers, const MosaicId& mosaicId, NotifyMode mode, const model::MatchedOffer* pMatchedOffer) {
			auto& offer = offers.at(mosaicId);
			if (NotifyMode::Commit == mode) {
				offer -= *pMatchedOffer;
			} else {
				offer += *pMatchedOffer;
			}
			return dynamic_cast<state::OfferBase&>(offer);
		}
	}

	DEFINE_OBSERVER(Exchange, model::ExchangeNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::ExchangeCache>();
		auto pMatchedOffer = notification.OffersPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pMatchedOffer) {
			auto mosaicId = context.Resolvers.resolve(pMatchedOffer->Mosaic.MosaicId);
			auto iter = cache.find(pMatchedOffer->Owner);
			auto& entry = iter.get();
			OfferExpiryUpdater offerExpiryUpdater(cache, entry);
			if (context.Mode == NotifyMode::Rollback && !entry.offerExists(pMatchedOffer->Type, mosaicId))
				entry.unexpireOffer(pMatchedOffer->Type, mosaicId, context.Height);
			auto& offer = (model::OfferType::Buy == pMatchedOffer->Type) ?
				ModifyOffer(entry.buyOffers(), mosaicId, context.Mode, pMatchedOffer) :
				ModifyOffer(entry.sellOffers(), mosaicId, context.Mode, pMatchedOffer);
			if (context.Mode == NotifyMode::Commit && Amount(0) == offer.Amount)
				entry.expireOffer(pMatchedOffer->Type, mosaicId, context.Height);
		}
	});
}}
