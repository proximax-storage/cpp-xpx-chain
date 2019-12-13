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
			auto iter = cache.find(notification.Owner);
			auto& entry = iter.get();
			OfferExpiryUpdater offerExpiryUpdater(cache, entry);

			const auto* pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				auto mosaicId = context.Resolvers.resolve(pOffer->Mosaic.MosaicId);
				auto deadline = GetOfferDeadline(pOffer->Duration, context.Height);
				entry.addOffer(mosaicId, pOffer, deadline);
			}
		} else {
			auto iter = cache.find(notification.Owner);
			auto& entry = iter.get();
			OfferExpiryUpdater offerExpiryUpdater(cache, entry);

			auto pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				auto mosaicId = context.Resolvers.resolve(pOffer->Mosaic.MosaicId);
				entry.removeOffer(pOffer->Type, mosaicId);
			}
		}
	}));
}}
