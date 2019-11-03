/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace validators {

	using Notification = model::RemoveOfferNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(RemoveOffer, [](const Notification& notification, const ValidatorContext& context) {
		if (notification.OfferCount == 0)
			return Failure_Exchange_No_Offered_Mosaics_To_Remove;

		auto& cache = context.Cache.sub<cache::ExchangeCache>();
		if (!cache.contains(notification.Owner))
			return Failure_Exchange_Account_Doesnt_Have_Any_Offer;

		auto iter = cache.find(notification.Owner);
		const auto& entry = iter.get();
		auto pOffer = notification.OffersPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
			auto mosaicId = context.Resolvers.resolve(pOffer->MosaicId);
			if (model::OfferType::Buy == pOffer->OfferType) {
				if (!entry.buyOffers().count(mosaicId))
					return Failure_Exchange_Offer_Doesnt_Exist;
			} else {
				if (!entry.sellOffers().count(mosaicId))
					return Failure_Exchange_Offer_Doesnt_Exist;
			}
		}

		return ValidationResult::Success;
	});
}}
