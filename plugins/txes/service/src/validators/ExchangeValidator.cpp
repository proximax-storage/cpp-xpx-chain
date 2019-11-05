/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "src/cache/DriveCache.h"
#include "src/utils/ServiceUtils.h"
#include "plugins/txes/exchange/src/cache/ExchangeCache.h"
#include "plugins/txes/exchange/src/config/ExchangeConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::ExchangeNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(Exchange, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(Exchange, [pConfigHolder](const Notification& notification, const ValidatorContext& context) {
            const auto& driveCache = context.Cache.sub<cache::DriveCache>();
            if (!driveCache.contains(notification.Signer))
                return ValidationResult::Success;

            auto driveIter = driveCache.find(notification.Signer);
            const auto& driveEntry = driveIter.get();

            if (driveEntry.state() != state::DriveState::Pending)
                return Failure_Service_Drive_Not_In_pending_State;

            if (driveEntry.processedDuration() >= driveEntry.duration())
                return Failure_Service_Drive_Processed_Full_Duration;

            const auto& config = pConfigHolder->Config(context.Height);
            const auto& pluginConfig = config.Network.GetPluginConfiguration<config::ExchangeConfiguration>(PLUGIN_NAME_HASH(exchange));
            const auto& defaultOfferPublicKey = pluginConfig.LongOfferKey;
            const auto& storageMosaicId = pConfigHolder->Config(context.Height).Immutable.StorageMosaicId;

            const auto& exchangeCache = context.Cache.sub<cache::ExchangeCache>();
            if (!exchangeCache.contains(defaultOfferPublicKey))
                return Failure_Service_Drive_Cant_Find_Default_Exchange_Offer;

            auto exchangeIter = exchangeCache.find(defaultOfferPublicKey);
            const auto& exchangeEntry = exchangeIter.get();

            if (!exchangeEntry.sellOffers().count(storageMosaicId))
                return Failure_Service_Drive_Cant_Find_Default_Exchange_Offer;

            Amount requiredAmount = driveEntry.billingPrice();
            auto billingBalance = utils::GetBillingBalanceOfDrive(driveEntry, context.Cache, storageMosaicId);
            if (billingBalance >= requiredAmount) {
                requiredAmount = Amount(1);
            } else {
                requiredAmount = requiredAmount - billingBalance;
            }

            const auto& sellOffer = exchangeEntry.sellOffers().at(storageMosaicId);
            if (sellOffer.Amount < requiredAmount)
                return Failure_Service_Drive_Cant_Find_Default_Exchange_Offer;

            Amount cost;
            Amount amount;

            const auto* pMatchedOffer = notification.OffersPtr;
            for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pMatchedOffer) {
                auto mosaicId = context.Resolvers.resolve(pMatchedOffer->Mosaic.MosaicId);

                if (mosaicId != storageMosaicId || pMatchedOffer->Type != model::OfferType::Sell)
                    return Failure_Service_Exchange_Of_This_Mosaic_Is_Not_Allowed;

                cost = cost + pMatchedOffer->Cost;
                amount = amount + pMatchedOffer->Mosaic.Amount;
            }

            if (amount > requiredAmount)
                return Failure_Service_Exchange_More_Than_Required;

            if (cost > sellOffer.cost(requiredAmount))
                return Failure_Service_Exchange_Cost_Is_Worse_Than_Default;

            return ValidationResult::Success;
		});
	};
}}
