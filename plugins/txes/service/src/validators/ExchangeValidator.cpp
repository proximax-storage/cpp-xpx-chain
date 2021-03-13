/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "src/utils/ServiceUtils.h"
#include "plugins/txes/exchange/src/cache/ExchangeCache.h"
#include "plugins/txes/exchange/src/config/ExchangeConfiguration.h"

namespace catapult { namespace validators {

	namespace {
		template<typename Notification>
		ValidationResult validate_v1(const Notification& notification, const StatefulValidatorContext& context) {
			const auto& driveCache = context.Cache.sub<cache::DriveCache>();
			if (!driveCache.contains(notification.Signer))
				return ValidationResult::Success;

			auto driveIter = driveCache.find(notification.Signer);
			const auto& driveEntry = driveIter.get();

			if (driveEntry.state() != state::DriveState::Pending)
				return Failure_Service_Drive_Not_In_Pending_State;

			if (driveEntry.processedDuration() >= driveEntry.duration())
				return Failure_Service_Drive_Processed_Full_Duration;

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::ExchangeConfiguration>();
			const auto& defaultOfferPublicKey = pluginConfig.LongOfferKey;
			const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;

			const auto& exchangeCache = context.Cache.sub<cache::ExchangeCache>();
			if (!exchangeCache.contains(defaultOfferPublicKey))
				return Failure_Service_Drive_Cant_Find_Default_Exchange_Offer;

			auto exchangeIter = exchangeCache.find(defaultOfferPublicKey);
			const auto& exchangeEntry = exchangeIter.get();

			if (!exchangeEntry.sellOffers().count(storageMosaicId))
				return Failure_Service_Drive_Cant_Find_Default_Exchange_Offer;

			Amount requiredAmount = driveEntry.billingPrice();
			auto driveBalance = utils::GetDriveBalance(driveEntry, context.Cache, storageMosaicId);
			if (driveBalance >= requiredAmount) {
				requiredAmount = Amount(1);
			} else {
				requiredAmount = requiredAmount - driveBalance;
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
		}
	}

	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(ExchangeV1, model::ExchangeNotification<1>, validate_v1<model::ExchangeNotification<1>>)
	DEFINE_STATEFUL_VALIDATOR_WITH_TYPE(ExchangeV2, model::ExchangeNotification<2>, validate_v1<model::ExchangeNotification<2>>)
}}
