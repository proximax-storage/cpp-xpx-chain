/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/OfferCache.h"

namespace catapult { namespace validators {

	using Notification = model::MatchedOfferNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(MatchedOffer, [](const Notification& notification, const ValidatorContext& context) {
		const auto& offerCache = context.Cache.sub<cache::OfferCache>();

		const auto* pOffer = notification.MatchedOffersPtr;
		for (uint8_t i = 0; i < notification.MatchedOfferCount; ++i, ++pOffer) {
			if (!offerCache.contains(pOffer->TransactionHash))
				return Failure_Exchange_Offer_Doesnt_Exist;

			const auto& matchedOfferEntry = offerCache.find(pOffer->TransactionHash).get();
			if (matchedOfferEntry.expiryHeight() < context.Height)
				return Failure_Exchange_Offer_Expired;

			if (matchedOfferEntry.offerType() == notification.OfferType)
				return Failure_Exchange_Invalid_Mathed_Offer_Type;

			if (matchedOfferEntry.transactionSigner() == notification.Signer)
				return Failure_Exchange_Buying_Own_Units_Is_Not_Allowed;

			const auto& offerIter = matchedOfferEntry.offers().find(pOffer->Mosaic.MosaicId);
			if (offerIter == matchedOfferEntry.offers().end())
				return Failure_Exchange_Unit_Not_Found_In_Offer;

			if (offerIter->second.Mosaic.Amount < pOffer->Mosaic.Amount)
				return Failure_Exchange_Not_Enough_Units_In_Offer;

			if (((offerIter->second.Mosaic.Amount == pOffer->Mosaic.Amount) && (offerIter->second.Cost != pOffer->Cost)) ||
				(offerIter->second.cost(pOffer->Mosaic.Amount) != pOffer->Cost))
				return Failure_Exchange_Invalid_Price;
		}

		return ValidationResult::Success;
	});
}}
