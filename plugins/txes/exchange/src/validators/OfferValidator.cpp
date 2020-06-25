/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace validators {

	namespace {
		template<VersionType version>
		ValidationResult OfferValidator(const model::OfferNotification<version> &notification, const ValidatorContext &context) {
			if (notification.OfferCount == 0)
				return Failure_Exchange_No_Offers;

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::ExchangeConfiguration>();

			std::set<MosaicId> offeredMosaicIds;
			auto* pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				auto mosaicId = context.Resolvers.resolve(pOffer->Mosaic.MosaicId);
				if (offeredMosaicIds.count(mosaicId))
					return Failure_Exchange_Duplicated_Offer_In_Request;

				offeredMosaicIds.insert(mosaicId);

				if (BlockDuration(0) == pOffer->Duration)
					return Failure_Exchange_Zero_Offer_Duration;

				if (pOffer->Duration > pluginConfig.MaxOfferDuration && notification.Owner != pluginConfig.LongOfferKey)
					return Failure_Exchange_Offer_Duration_Too_Large;

				if (pOffer->Mosaic.Amount == Amount(0))
					return Failure_Exchange_Zero_Amount;

				if (pOffer->Cost == Amount(0))
					return Failure_Exchange_Zero_Price;

				if (mosaicId == context.Config.Immutable.CurrencyMosaicId)
					return Failure_Exchange_Mosaic_Not_Allowed;
			}

			const auto& cache = context.Cache.sub<cache::ExchangeCache>();

			if (!cache.contains(notification.Owner))
				return ValidationResult::Success;

			auto iter = cache.find(notification.Owner);
			const auto& entry = iter.get();
			pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				auto mosaicId = context.Resolvers.resolve(pOffer->Mosaic.MosaicId);
				if (entry.offerExists(pOffer->Type, mosaicId))
					return Failure_Exchange_Offer_Exists;
			}

			return ValidationResult::Success;
		}
	}

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(OfferV1, model::OfferNotification<1>, OfferValidator<1>)
	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(OfferV2, model::OfferNotification<2>, OfferValidator<2>)
	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(OfferV3, model::OfferNotification<3>, OfferValidator<3>)
}}
