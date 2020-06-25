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

	template<VersionType version>
	void OfferObserver(const model::OfferNotification<version>& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<cache::ExchangeCache>();
		if (NotifyMode::Commit == context.Mode && !cache.contains(notification.Owner))
			cache.insert(state::ExchangeEntry(notification.Owner, version));
		auto iter = cache.find(notification.Owner);
		auto& entry = iter.get();
		OfferExpiryUpdater offerExpiryUpdater(cache, entry);

		const auto* pOffer = notification.OffersPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
			auto mosaicId = context.Resolvers.resolve(pOffer->Mosaic.MosaicId);
			if (NotifyMode::Commit == context.Mode) {
				auto deadline = GetOfferDeadline(pOffer->Duration, context.Height);
				entry.addOffer(mosaicId, pOffer, deadline);
			} else {
				entry.removeOffer(pOffer->Type, mosaicId);
			}
		}

		if (NotifyMode::Rollback == context.Mode && entry.empty() && entry.version() > 2)
			cache.remove(notification.Owner);
	}

	DEFINE_OBSERVER(OfferV1, model::OfferNotification<1>, OfferObserver<1>);
	DEFINE_OBSERVER(OfferV2, model::OfferNotification<2>, OfferObserver<2>);
	DEFINE_OBSERVER(OfferV3, model::OfferNotification<3>, OfferObserver<3>);
}}
