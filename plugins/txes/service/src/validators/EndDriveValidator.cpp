/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "catapult/validators/ValidatorContext.h"
#include "plugins/txes/exchange/src/cache/ExchangeCache.h"
#include "plugins/txes/exchange/src/config/ExchangeConfiguration.h"
#include "src/utils/ServiceUtils.h"

namespace catapult { namespace validators {

	using Notification = model::EndDriveNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(EndDrive, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(EndDrive, [](const Notification &notification, const ValidatorContext &context) {
			const auto &driveCache = context.Cache.sub<cache::DriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			const auto &driveEntry = driveIter.get();

			if (driveEntry.owner() == notification.Signer) {
				return ValidationResult::Success;
			} else if (driveEntry.key() == notification.Signer) {
				if (driveEntry.state() != state::DriveState::Pending)
					return Failure_Service_Drive_Not_In_Pending_State;

				if (driveEntry.processedDuration() >= driveEntry.duration())
					return ValidationResult::Success;

				const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::ExchangeConfiguration>();

				const auto& defaultOfferPublicKey = pluginConfig.LongOfferKey;
				const auto& storageMosaicId = context.Config.Immutable.StorageMosaicId;
				const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;

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

				const auto& accountCache = context.Cache.sub<cache::AccountStateCache>();
				auto accountIter = accountCache.find(driveEntry.key());
				const auto& driveAccount = accountIter.get();

				auto currencyBalance = driveAccount.Balances.get(currencyMosaicId);
				if (currencyBalance < sellOffer.cost(requiredAmount))
					return ValidationResult::Success;
			}

			return Failure_Service_Operation_Is_Not_Permitted;
		});
	};
}}
