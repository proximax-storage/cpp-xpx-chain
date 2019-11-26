/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace validators {

	namespace {
		template<typename TOffer>
		bool OfferCostInvalid(TOffer& offer, const model::MatchedOffer* pMatchedOffer) {
			return ((offer.InitialAmount == pMatchedOffer->Mosaic.Amount) && (offer.InitialCost != pMatchedOffer->Cost)) ||
				(offer.cost(pMatchedOffer->Mosaic.Amount) != pMatchedOffer->Cost);
		}
	}

	using Notification = model::ExchangeNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(Exchange, [](const Notification& notification, const ValidatorContext& context) {
		if (notification.OfferCount == 0)
			return Failure_Exchange_No_Offers;

		const auto& cache = context.Cache.sub<cache::ExchangeCache>();

		const auto* pMatchedOffer = notification.OffersPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pMatchedOffer) {
			if (pMatchedOffer->Owner == notification.Signer)
				return Failure_Exchange_Buying_Own_Units_Is_Not_Allowed;

			if (!cache.contains(pMatchedOffer->Owner))
				return Failure_Exchange_Account_Doesnt_Have_Any_Offer;

			auto mosaicId = context.Resolvers.resolve(pMatchedOffer->Mosaic.MosaicId);
			auto iter = cache.find(pMatchedOffer->Owner);
			const auto& entry = iter.get();

			if (model::OfferType::Buy == pMatchedOffer->Type) {
				if (!entry.buyOffers().count(mosaicId))
					return Failure_Exchange_Offer_Doesnt_Exist;
				if (OfferCostInvalid(entry.buyOffers().at(mosaicId), pMatchedOffer))
					return Failure_Exchange_Invalid_Price;
			} else {
				if (!entry.sellOffers().count(mosaicId))
					return Failure_Exchange_Offer_Doesnt_Exist;
				if (OfferCostInvalid(entry.sellOffers().at(mosaicId), pMatchedOffer))
					return Failure_Exchange_Invalid_Price;
			}

			const auto& offer = (model::OfferType::Buy == pMatchedOffer->Type) ?
				dynamic_cast<const state::OfferBase&>(entry.buyOffers().at(mosaicId)) :
				dynamic_cast<const state::OfferBase&>(entry.sellOffers().at(mosaicId));

			if (offer.Amount < pMatchedOffer->Mosaic.Amount)
				return Failure_Exchange_Not_Enough_Units_In_Offer;
		}

		return ValidationResult::Success;
	});
}}
