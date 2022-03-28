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
        std::set<std::pair<MosaicId, MosaicId>> offeredMosaicIds;
        auto pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicIdGive = context.Resolvers.resolve(pSdaOffer->MosaicIdGive);
			auto mosaicIdGet = context.Resolvers.resolve(pSdaOffer->MosaicIdGet);

            std::pair<MosaicId, MosaicId> pair(mosaicIdGive, mosaicIdGet);
            if (offeredMosaicIds.count(pair))
                return Failure_ExchangeSda_Duplicated_Offer_In_Request;
            offeredMosaicIds.insert(pair);

            auto result = ValidationResult::Success;
            auto offers = entry.sdaOfferBalances();
            auto expiredOffers = entry.expiredSdaOfferBalances();
            const Height& height = context.Height;

            if (!offers.count(pair))
				return Failure_ExchangeSda_Offer_Doesnt_Exist;

			auto& offer = offers.at(pair);
			if (offer.Deadline <= height)
				return Failure_ExchangeSda_Offer_Expired;

			if (expiredOffers.count(height) && expiredOffers.at(height).count(pair))
				return Failure_ExchangeSda_Cant_Remove_Offer_At_Height;
        }

        return ValidationResult::Success;
    }));
}}