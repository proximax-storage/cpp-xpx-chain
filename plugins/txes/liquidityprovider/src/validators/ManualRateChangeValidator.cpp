/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::ManualRateChangeNotification<1>;

    DEFINE_STATEFUL_VALIDATOR(ManualRateChange, [](const Notification& notification, const ValidatorContext& context) {
        const auto& liquidityProviderCache = context.Cache.sub<cache::LiquidityProviderCache>();

 		const auto entryIter = liquidityProviderCache.find(notification.ProviderMosaicId);
		const auto* pEntry = entryIter.tryGet();

		if (!pEntry) {
			return Failure_LiquidityProvider_Liquidity_Provider_Is_Not_Registered;
		}

		if (notification.Signer != pEntry->owner()) {
			return Failure_LiquidityProvider_Invalid_Owner;
		}

		const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();

		auto lpStateEntryIter = accountStateCache.find(pEntry->providerKey());
		const auto& lpStateEntry = lpStateEntryIter.get();

		auto ownerStateEntryIter = accountStateCache.find(pEntry->owner());
		const auto& ownerStateEntry = ownerStateEntryIter.get();

		const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		if (notification.CurrencyBalanceIncrease) {
			auto ownerBalance = ownerStateEntry.Balances.get(currencyMosaicId);

			if (ownerBalance < notification.CurrencyBalanceChange) {
				return Failure_LiquidityProvider_Insufficient_Currency;
			}
		}
		else {
			auto lpBalance = lpStateEntry.Balances.get(currencyMosaicId);

			// We can not allow zero on LP balance
			if (lpBalance <= notification.CurrencyBalanceChange) {
				return Failure_LiquidityProvider_Invalid_Exchange_Rate;
			}
		}


		// We can not allow zero on LP balance
		if (!notification.MosaicBalanceIncrease) {
			auto mosaicId = context.Resolvers.resolve(pEntry->mosaicId());
			auto mosaicBalance = lpStateEntry.Balances.get(mosaicId);

			if (mosaicBalance <= notification.MosaicBalanceChange) {
				return Failure_LiquidityProvider_Invalid_Exchange_Rate;
			}
		}

        return ValidationResult::Success;
    })
}}
