/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace validators {

	namespace {
		template<typename TOffer>
		bool OfferCostInvalid(TOffer& offer, const model::MatchedOffer* pMatchedOffer) {
			return (offer.cost(pMatchedOffer->Mosaic.Amount) != pMatchedOffer->Cost);
		}

		template<typename TOfferMap, typename TExpiredOfferMap>
		ValidationResult ValidateOffer(
				const TOfferMap& offers,
				const TExpiredOfferMap& expiredOffers,
				const model::MatchedOffer* pMatchedOffer,
				const MosaicId& mosaicId,
				const Height& height) {
			if (!offers.count(mosaicId))
				return Failure_Exchange_Offer_Doesnt_Exist;

			auto& offer = offers.at(mosaicId);
			if (offer.Deadline <= height)
				return Failure_Exchange_Offer_Expired;

			if (OfferCostInvalid(offer, pMatchedOffer))
				return Failure_Exchange_Invalid_Price;

			if (offer.Amount < pMatchedOffer->Mosaic.Amount)
				return Failure_Exchange_Not_Enough_Units_In_Offer;

			// Check whether fulfilled offer can be moved to array of expired offers.
			if (offer.Amount == pMatchedOffer->Mosaic.Amount && expiredOffers.count(height) && expiredOffers.at(height).count(mosaicId))
				return Failure_Exchange_Cant_Remove_Offer_At_Height;

			return ValidationResult::Success;
		}
	}

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(ExchangeV1, model::ExchangeNotification<1>, ([](const model::ExchangeNotification<1>& notification, const ValidatorContext& context) {
		if (notification.OfferCount == 0)
			return Failure_Exchange_No_Offers;

		const auto& cache = context.Cache.sub<cache::ExchangeCache>();

		std::set<std::pair<Key, MosaicId>> offers;
		const auto* pMatchedOffer = notification.OffersPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pMatchedOffer) {
			if (pMatchedOffer->Owner == notification.Signer)
				return Failure_Exchange_Buying_Own_Units_Is_Not_Allowed;

			if (!cache.contains(pMatchedOffer->Owner))
				return Failure_Exchange_Account_Doesnt_Have_Any_Offer;

			auto mosaicId = context.Resolvers.resolve(pMatchedOffer->Mosaic.MosaicId);
			std::pair<Key, MosaicId> pair(pMatchedOffer->Owner, mosaicId);
			if (offers.count(pair))
				return Failure_Exchange_Duplicated_Offer_In_Request;
			offers.insert(pair);

			auto iter = cache.find(pMatchedOffer->Owner);
			const auto& entry = iter.get();

			auto result = ValidationResult::Success;
			if (model::OfferType::Buy == pMatchedOffer->Type) {
				result = ValidateOffer(entry.buyOffers(), entry.expiredBuyOffers(), pMatchedOffer, mosaicId, context.Height);
			} else {
				result = ValidateOffer(entry.sellOffers(), entry.expiredSellOffers(), pMatchedOffer, mosaicId, context.Height);
			}
			if (ValidationResult::Success != result)
				return result;
		}

		return ValidationResult::Success;
	}));

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(ExchangeV2, model::ExchangeNotification<2>, ([](const model::ExchangeNotification<2>& notification, const ValidatorContext& context) {
	  if (notification.OfferCount == 0)
		  return Failure_Exchange_No_Offers;

	  const auto& cache = context.Cache.sub<cache::ExchangeCache>();

	  std::set<std::pair<Key, MosaicId>> offers;
	  const auto* pMatchedOffer = notification.OffersPtr;
	  for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pMatchedOffer) {
		  if (pMatchedOffer->Owner == notification.Signer)
			  return Failure_Exchange_Buying_Own_Units_Is_Not_Allowed;

		  if (!cache.contains(pMatchedOffer->Owner))
			  return Failure_Exchange_Account_Doesnt_Have_Any_Offer;

		  auto mosaicId = context.Resolvers.resolve(pMatchedOffer->Mosaic.MosaicId);
		  std::pair<Key, MosaicId> pair(pMatchedOffer->Owner, mosaicId);
		  if (offers.count(pair))
			  return Failure_Exchange_Duplicated_Offer_In_Request;
		  offers.insert(pair);

		  auto iter = cache.find(pMatchedOffer->Owner);
		  const auto& entry = iter.get();

		  auto result = ValidationResult::Success;
		  if (model::OfferType::Buy == pMatchedOffer->Type) {
			  result = ValidateOffer(entry.buyOffers(), entry.expiredBuyOffers(), pMatchedOffer, mosaicId, context.Height);
		  } else if (model::OfferType::Sell == pMatchedOffer->Type) {
			  result = ValidateOffer(entry.sellOffers(), entry.expiredSellOffers(), pMatchedOffer, mosaicId, context.Height);
		  } else {
		  	return Failure_Exchange_Incorrect_Offer_Type;
		  }

		  if (ValidationResult::Success != result)
			  return result;
	  }

	  return ValidationResult::Success;
	}))
}}
