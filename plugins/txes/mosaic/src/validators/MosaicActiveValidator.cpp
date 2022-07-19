/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "ActiveMosaicView.h"
#include "src/cache/MosaicCache.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::MosaicActiveNotification<1>;

    DEFINE_STATEFUL_VALIDATOR(MosaicActive, [](const auto& notification, const ValidatorContext& context) {
        const auto& cache = context.Cache.sub<cache::MosaicCache>();
        MosaicId mosaicId = context.Resolvers.resolve(notification.MosaicId);

        // check if a mosaic is active
        if (!cache.isActive(mosaicId, context.Height))
            return Failure_Mosaic_Expired;
        
        auto mosaicIter = cache.find(mosaicId);
        const auto& mosaicEntry = mosaicIter.get();
        auto mosaicBlockDuration = mosaicEntry.definition().properties().duration();
        auto mosaicCreatedHeight = mosaicEntry.definition().height();

        Height maxMosaicHeight = mosaicCreatedHeight + Height(mosaicBlockDuration.unwrap());
        Height maxOfferHeight = notification.OfferExpirationHeight;

        // check if an offer's lifetime exceeds a mosaic's lifetime
        if (!mosaicEntry.definition().isEternal() && maxOfferHeight > maxMosaicHeight)
            return Failure_Mosaic_Offer_Duration_Exceeds_Mosaic_Duration;

        return ValidationResult::Success;
    });
}}