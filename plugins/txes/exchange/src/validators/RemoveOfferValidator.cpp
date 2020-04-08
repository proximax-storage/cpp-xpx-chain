/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace validators {

	namespace {
		template<typename TOfferMap, typename TExpiredOfferMap>
		ValidationResult ValidateOffer(const TOfferMap& offers, const TExpiredOfferMap& expiredOffers, const MosaicId& mosaicId, const Height& height) {
			if (!offers.count(mosaicId))
				return Failure_Exchange_Offer_Doesnt_Exist;

			auto& offer = offers.at(mosaicId);
			if (offer.Deadline <= height)
				return Failure_Exchange_Offer_Expired;

			if (expiredOffers.count(height) && expiredOffers.at(height).count(mosaicId))
				return Failure_Exchange_Cant_Remove_Offer_At_Height;

			return ValidationResult::Success;
		}
	}

	using Notification = model::RemoveOfferNotification<1>;

	DEFINE_STATEFUL_VALIDATOR(RemoveOffer, ([](const Notification& notification, const ValidatorContext& context) {
		if (notification.OfferCount == 0)
			return Failure_Exchange_No_Offered_Mosaics_To_Remove;

		auto& cache = context.Cache.sub<cache::ExchangeCache>();
		if (!cache.contains(notification.Owner))
			return Failure_Exchange_Account_Doesnt_Have_Any_Offer;

		auto iter = cache.find(notification.Owner);
		const auto& entry = iter.get();
		std::set<std::pair<model::OfferType, MosaicId>> offers;
		auto pOffer = notification.OffersPtr;
		for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
			auto mosaicId = context.Resolvers.resolve(pOffer->MosaicId);

			std::pair<model::OfferType, MosaicId> pair(pOffer->OfferType, mosaicId);
			if (offers.count(pair))
				return Failure_Exchange_Duplicated_Offer_In_Request;
			offers.insert(pair);

			auto result = ValidationResult::Success;
			if (model::OfferType::Buy == pOffer->OfferType) {
				result = ValidateOffer(entry.buyOffers(), entry.expiredBuyOffers(), mosaicId, context.Height);
			} else {
				result = ValidateOffer(entry.sellOffers(), entry.expiredSellOffers(), mosaicId, context.Height);
			}
			if (ValidationResult::Success != result)
				return result;
		}

		return ValidationResult::Success;
	}));
}}
