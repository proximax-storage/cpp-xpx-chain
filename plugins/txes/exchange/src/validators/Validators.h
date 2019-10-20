/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "Results.h"
#include "src/model/ExchangeNotifications.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/validators/ValidatorTypes.h"

namespace catapult { namespace validators {

	template<typename TNotification>
	ValidationResult ValidateOffers(const TNotification& notification, const ValidatorContext& context, std::set<UnresolvedMosaicId> allowedMosaicIds) {
		if (notification.OfferCount == 0)
			return Failure_Exchange_No_Offers;

		const model::Offer* pOffer = notification.OffersPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
			if (pOffer->Mosaic.Amount == Amount(0))
				return Failure_Exchange_Zero_Amount;

			if (pOffer->Cost == Amount(0))
				return Failure_Exchange_Zero_Price;

			if (notification.Deadline <= context.BlockTime)
				return Failure_Exchange_Past_Offer_Deadline;

			if (!allowedMosaicIds.count(pOffer->Mosaic.MosaicId))
				return Failure_Exchange_Invalid_Mosaic;
		}

		return ValidationResult::Success;
	}

	/// A validator implementation that applies to remove offer notification and validates that:
	/// - offer exists
	/// - offer remove signer is offer owner
	DECLARE_STATEFUL_VALIDATOR(BuyOffer, model::BuyOfferNotification<1>)(const config::ImmutableConfiguration& config);

	/// A validator implementation that applies to remove offer notification and validates that:
	/// - offer exists
	/// - offer remove signer is offer owner
	DECLARE_STATEFUL_VALIDATOR(SellOffer, model::SellOfferNotification<1>)(const config::ImmutableConfiguration& config);

	template<typename TNotification, typename TCache>
	ValidationResult MatchedOfferValidator(const TNotification& notification, const ValidatorContext& context) {
		auto& matchedOfferCache = context.Cache.sub<TCache>();

		const auto* pOffer = notification.MatchedOffersPtr;
		for (uint8_t i = 0; i < notification.MatchedOfferCount; ++i, ++pOffer) {
			if (!matchedOfferCache.contains(pOffer->TransactionHash))
				return Failure_Exchange_Offer_Doesnt_Exist;

			const auto& matchedOfferEntry = matchedOfferCache.find(pOffer->TransactionHash).get();
			if (matchedOfferEntry.transactionSigner() == notification.Signer)
				return Failure_Exchange_Buying_Own_Units_Is_Not_Allowed;

			const auto& offerIter = matchedOfferEntry.offers().find(pOffer->Mosaic.MosaicId);
			if (offerIter == matchedOfferEntry.offers().end())
				return Failure_Exchange_Unit_Not_Found_In_Offer;

			if (offerIter->second.Mosaic.Amount < pOffer->Mosaic.Amount)
				return Failure_Exchange_Not_Enough_Units_In_Offer;

			if ((offerIter->second.Mosaic.Amount == pOffer->Mosaic.Amount && offerIter->second.Cost != pOffer->Cost) ||
				Amount(offerIter->second.price() * pOffer->Mosaic.Amount.unwrap()) != pOffer->Cost)
				return Failure_Exchange_Invalid_Price;
		}

		return ValidationResult::Success;
	}

	/// A validator implementation that applies to remove offer notification and validates that:
	/// - offer exists
	/// - offer remove signer is offer owner
	DECLARE_STATEFUL_VALIDATOR(MatchedBuyOffer, model::MatchedBuyOfferNotification<1>)();

	/// A validator implementation that applies to remove offer notification and validates that:
	/// - offer exists
	/// - offer remove signer is offer owner
	DECLARE_STATEFUL_VALIDATOR(MatchedSellOffer, model::MatchedSellOfferNotification<1>)();

	/// A validator implementation that applies to remove offer notification and validates that:
	/// - offer exists
	/// - offer remove signer is offer owner
	DECLARE_STATEFUL_VALIDATOR(RemoveOffer, model::RemoveOfferNotification<1>)();
}}
