/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/ExchangeCache.h"
#include "src/config/ExchangeConfiguration.h"

namespace catapult { namespace validators {

	namespace {
		std::set<UnresolvedMosaicId> GetAllowedMosaicIds(const config::ImmutableConfiguration& config) {
			return {
				config::GetUnresolvedStorageMosaicId(config),
				config::GetUnresolvedStreamingMosaicId(config),
				config::GetUnresolvedReviewMosaicId(config),
				config::GetUnresolvedSuperContractMosaicId(config)
			};
		}
	}

	using Notification = model::OfferNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(Offer, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(Offer, [pConfigHolder](const Notification& notification, const ValidatorContext& context) {
			if (notification.OfferCount == 0)
				return Failure_Exchange_No_Offers;

			auto& cache = context.Cache.sub<cache::ExchangeCache>();
			auto iter = cache.find(notification.Owner);
			const auto& entry = iter.get();

			const auto& config = pConfigHolder->Config(context.Height);
			const auto& pluginConfig = config.Network.GetPluginConfiguration<config::ExchangeConfiguration>(PLUGIN_NAME_HASH(exchange));

			auto allowedMosaicIds = GetAllowedMosaicIds(config.Immutable);
			std::set<MosaicId> offeredMosaicIds;
			auto* pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				auto mosaicId = context.Resolvers.resolve(pOffer->Mosaic.MosaicId);
				if (entry.offerExists(pOffer->Type, mosaicId))
					return Failure_Exchange_Offer_Exists;

				if (!offeredMosaicIds.count(mosaicId))
					return Failure_Exchange_Duplicated_Offer_In_Request;
				offeredMosaicIds.insert(mosaicId);

				if (pOffer->Duration > pluginConfig.MaxOfferDuration && notification.Owner != context.Network.PublicKey) {
					return Failure_Exchange_Offer_Duration_Too_Large;
				}

				if (pOffer->Mosaic.Amount == Amount(0))
					return Failure_Exchange_Zero_Amount;

				if (pOffer->Cost == Amount(0))
					return Failure_Exchange_Zero_Price;

				if (!allowedMosaicIds.count(pOffer->Mosaic.MosaicId))
					return Failure_Exchange_Mosaic_Not_Allowed;
			}

			return ValidationResult::Success;
		})
	}
}}
