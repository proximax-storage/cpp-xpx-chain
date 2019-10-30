/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
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

			const auto& config = pConfigHolder->Config(context.Height);
			const auto& pluginConfig = config.Network.GetPluginConfiguration<config::ExchangeConfiguration>(PLUGIN_NAME_HASH(exchange));

			auto allowedMosaicIds = GetAllowedMosaicIds(config.Immutable);
			const model::Offer* pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				if (pOffer->Duration > pluginConfig.MaxOfferDuration && notification.Signer != context.Network.PublicKey) {
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
