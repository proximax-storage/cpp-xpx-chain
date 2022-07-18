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
        ValidationResult ValidateMosaicId(const ValidatorContext &context, const MosaicId& mosaicId, const BlockDuration& duration) {
            const auto& mosaicCache = context.Cache.sub<cache::MosaicCache>();

            if (!mosaicCache.contains(mosaicId))
                return Failure_ExchangeSda_Mosaic_Not_Found;

            auto mosaicEntryIter = mosaicCache.find(mosaicId);
            const auto& mosaicEntry = mosaicEntryIter.get();
            auto mosaicBlockDuration = mosaicEntry.definition().properties().duration();
            auto mosaicCreatedHeight = mosaicEntry.definition().height();

            Height maxMosaicHeight = mosaicCreatedHeight + Height(mosaicBlockDuration.unwrap());
            Height maxOfferHeight = Height(context.Height.unwrap() + duration.unwrap());

            if (!mosaicEntry.definition().isEternal() && maxOfferHeight > maxMosaicHeight)
                return Failure_ExchangeSda_Offer_Duration_Exceeds_Mosaic_Duration;

            return ValidationResult::Success;
        }
    }

    DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(PlaceSdaExchangeOfferV1, model::PlaceSdaOfferNotification<1>, ([](const model::PlaceSdaOfferNotification<1>& notification, const ValidatorContext& context) {
        if (notification.SdaOfferCount == 0)
            return Failure_ExchangeSda_No_Offers;
        
        const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::SdaExchangeConfiguration>();

        std::set<state::MosaicsPair> offers;
        auto* pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicIdGive = context.Resolvers.resolve(pSdaOffer->MosaicGive.MosaicId);
            auto mosaicIdGet = context.Resolvers.resolve(pSdaOffer->MosaicGet.MosaicId);

            if (mosaicIdGive == mosaicIdGet)
                return Failure_ExchangeSda_Exchanging_Same_Units_Is_Not_Allowed;

            state::MosaicsPair pair(mosaicIdGive, mosaicIdGet);
            if (offers.count(pair))
                return Failure_ExchangeSda_Duplicated_Offer_In_Request;
            offers.insert(pair);

            if (BlockDuration(0) == pSdaOffer->Duration)
                return Failure_ExchangeSda_Zero_Offer_Duration;

            if (pSdaOffer->Duration > pluginConfig.MaxOfferDuration && notification.Signer != pluginConfig.LongOfferKey)
                return Failure_ExchangeSda_Offer_Duration_Too_Large;
            
            auto result = ValidationResult::Success;
            result = ValidateMosaicId(context, mosaicIdGive, pSdaOffer->Duration);
            if (ValidationResult::Success != result) return result;
            result = ValidateMosaicId(context, mosaicIdGet, pSdaOffer->Duration);
            if (ValidationResult::Success != result) return result;

            if (pSdaOffer->MosaicGive.Amount == Amount(0))
                return Failure_ExchangeSda_Zero_Amount;

            if (pSdaOffer->MosaicGet.Amount == Amount(0))
                return Failure_ExchangeSda_Zero_Price;
        }

        const auto& cache = context.Cache.sub<cache::SdaExchangeCache>();

        if (!cache.contains(notification.Signer))
            return ValidationResult::Success;

        auto iter = cache.find(notification.Signer);
        const auto& entry = iter.get();
        pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicIdGive = context.Resolvers.resolve(pSdaOffer->MosaicGive.MosaicId);
            auto mosaicIdGet = context.Resolvers.resolve(pSdaOffer->MosaicGet.MosaicId);
            state::MosaicsPair pair(mosaicIdGive, mosaicIdGet);
            if (entry.offerExists(pair))
                return Failure_ExchangeSda_Offer_Exists;
        }

        return ValidationResult::Success;
    }))
}}