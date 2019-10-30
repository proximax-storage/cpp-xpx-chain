/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	namespace {
		Height GetOfferDeadline(const BlockDuration& duration, const Height& currentHeight) {
			auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
			return Height((duration.unwrap() < maxDeadline - currentHeight.unwrap()) ? currentHeight.unwrap() + duration.unwrap() : maxDeadline);
		}
	}

	DEFINE_OBSERVER(Offer, model::OfferNotification<1>, ([](const auto& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::ExchangeCache>();
		if (NotifyMode::Commit == context.Mode) {
			if (!cache.contains(notification.Owner))
				cache.insert(state::ExchangeEntry(notification.Owner));
			auto& entry = cache.find(notification.Owner).get();

			const model::OfferWithDuration* pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				auto mosaicId = context.Resolvers.resolve(pOffer->Mosaic.MosaicId);
				auto deadline = GetOfferDeadline(pOffer->Duration, context.Height);
				state::OfferBase baseOffer{pOffer->Mosaic.Amount, pOffer->Mosaic.Amount, pOffer->Cost, deadline, deadline};
				if (model::OfferType::Buy == pOffer->Type) {
					entry.buyOffers().emplace(mosaicId, state::BuyOffer{baseOffer, pOffer->Cost});
				} else {
					entry.sellOffers().emplace(mosaicId, state::SellOffer{baseOffer});
				}
			}
		} else {
			auto& entry = cache.find(notification.Owner).get();

			auto pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				auto mosaicId = context.Resolvers.resolve(pOffer->Mosaic.MosaicId);
				if (model::OfferType::Buy == pOffer->Type) {
					entry.buyOffers().erase(mosaicId);
				} else {
					entry.sellOffers().erase(mosaicId);
				}
			}

			if (entry.sellOffers().empty() && entry.buyOffers().empty())
				cache.remove(notification.Owner);
		}
	}));
}}
