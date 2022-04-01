/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "plugins/txes/mosaic/src/cache/MosaicCache.h"
#include "Validators.h"
#include "src/cache/SdaExchangeCache.h"

namespace catapult { namespace validators {

    namespace {
		ValidationResult ValidateSdaOffer(const state::SdaOfferBalanceMap& offers, const state::ExpiredSdaOfferBalanceMap& expiredOffers, const model::SdaOfferWithOwnerAndDuration* pSdaOffer, const state::MosaicsPair& mosaicId, const Height& height) {
			if (!offers.count(mosaicId))
				return Failure_ExchangeSda_Offer_Doesnt_Exist;

			auto& offer = offers.at(mosaicId);
			if (offer.Deadline <= height)
				return Failure_ExchangeSda_Offer_Expired;

			if (offer.CurrentMosaicGive < pSdaOffer->MosaicGive.Amount)
				return Failure_ExchangeSda_Not_Enough_Units_In_Offer;

			// Check whether fulfilled offer can be moved to array of expired offers.
			if (offer.CurrentMosaicGive == pSdaOffer->MosaicGive.Amount && expiredOffers.count(height) && expiredOffers.at(height).count(mosaicId))
				return Failure_ExchangeSda_Cant_Remove_Offer_At_Height;

			return ValidationResult::Success;
		}
    }

    DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(PlaceSdaExchangeOfferV1, model::PlaceSdaOfferNotification<1>, ([](const model::PlaceSdaOfferNotification<1>& notification, const ValidatorContext& context) {
        if (notification.SdaOfferCount == 0)
		    return Failure_ExchangeSda_No_Offers;
        
        const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::SdaExchangeConfiguration>();
        const auto& cache = context.Cache.sub<cache::SdaExchangeCache>();
	    const auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();

        std::set<state::MosaicsPair> offers;
	    const auto* pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicIdGive = context.Resolvers.resolve(pSdaOffer->MosaicGive.MosaicId);
			auto mosaicIdGet = context.Resolvers.resolve(pSdaOffer->MosaicGet.MosaicId);

            state::MosaicsPair pair(mosaicIdGive, mosaicIdGet);
            if (offers.count(pair))
				return Failure_ExchangeSda_Duplicated_Offer_In_Request;
			offers.insert(pair);

            if (BlockDuration(0) == pSdaOffer->Duration)
			    return Failure_ExchangeSda_Zero_Offer_Duration;

		    if (pSdaOffer->Duration > pluginConfig.MaxOfferDuration && notification.Signer != pluginConfig.LongOfferKey)
			    return Failure_ExchangeSda_Offer_Duration_Too_Large;
            
            auto mosaicEntryIter = mosaicCache.find(mosaicIdGive);
		    const auto& mosaicEntry = mosaicEntryIter.get();
		    auto mosaicBlockDuration = mosaicEntry.definition().properties().duration();

            Height maxMosaicHeight = Height(context.Height.unwrap() + mosaicBlockDuration.unwrap());
		    Height maxOfferHeight = Height(context.Height.unwrap() + pSdaOffer->Duration.unwrap());

            if (!mosaicEntry.definition().isEternal() && maxOfferHeight > maxMosaicHeight)
                return Failure_ExchangeSda_Offer_Duration_Exceeds_Mosaic_Duration;

            if (pSdaOffer->MosaicGive.Amount == Amount(0))
                return Failure_ExchangeSda_Zero_Amount;

            if (pSdaOffer->MosaicGet.Amount == Amount(0))
                return Failure_ExchangeSda_Zero_Price;

            auto iter = cache.find(notification.Signer);
			const auto& entry = iter.get();
            if (entry.offerExists(pair))
			    return Failure_ExchangeSda_Offer_Exists;

            auto result = ValidationResult::Success;
            result = ValidateSdaOffer(entry.sdaOfferBalances(), entry.expiredSdaOfferBalances(), pSdaOffer, pair, context.Height);
            if (ValidationResult::Success != result)
				return result;
        }

        if (!cache.contains(notification.Signer))
		    return ValidationResult::Success;
    }));
}}