/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/SdaExchangeCache.h"

namespace catapult { namespace validators {

    DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(RemoveSdaExchangeOfferV1, model::RemoveSdaOfferNotification<1>, ([](const model::RemoveSdaOfferNotification<1>& notification, const ValidatorContext& context) {
        if (notification.SdaOfferCount == 0)
			return Failure_ExchangeSda_No_Offered_Mosaics_To_Remove;

        auto& cache = context.Cache.sub<cache::SdaExchangeCache>();
		if (!cache.contains(notification.Owner))
			return Failure_ExchangeSda_Account_Doesnt_Have_Any_Offer;

        auto iter = cache.find(notification.Owner);
        const auto& entry = iter.get();
        std::set<MosaicId> offeredMosaicIds;
        auto pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicId = context.Resolvers.resolve(pSdaOffer->MosaicId);
            if (offeredMosaicIds.count(mosaicId))
                return Failure_ExchangeSda_Duplicated_Offer_In_Request;
            offeredMosaicIds.insert(mosaicId);

            auto result = ValidationResult::Success;
            auto offers = entry.swapOffers();
            auto expiredOffers = entry.expiredSwapOffers();
            const Height& height = context.Height;

            if (!offers.count(mosaicId))
				return Failure_ExchangeSda_Offer_Doesnt_Exist;

			auto& offer = offers.at(mosaicId);
			if (offer.Deadline <= height)
				return Failure_ExchangeSda_Offer_Expired;

			if (expiredOffers.count(height) && expiredOffers.at(height).count(mosaicId))
				return Failure_ExchangeSda_Cant_Remove_Offer_At_Height;
        }

        return ValidationResult::Success;
    }));
}}