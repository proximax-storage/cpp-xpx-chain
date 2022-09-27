/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::CreateLiquidityProviderNotification<1>;

    DEFINE_STATEFUL_VALIDATOR(CreateLiquidityProvider, [](const Notification& notification, const ValidatorContext& context) {
        const auto& liquidityProviderCache = context.Cache.sub<cache::LiquidityProviderCache>();

		if(notification.ProviderMosaicId == UnresolvedMosaicId()) {
			return Failure_LiquidityProvider_Reserved_Mosaic_Id;
		}

		if(notification.ProviderMosaicId == config::GetUnresolvedCurrencyMosaicId(context.Config.Immutable)) {
			return Failure_LiquidityProvider_Reserved_Mosaic_Id;
		}

		if (liquidityProviderCache.contains(notification.ProviderMosaicId)) {
			return Failure_LiquidityProvider_Liquidity_Provider_Already_Exists;
		}

		const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::LiquidityProviderConfiguration>();

		if (pluginConfig.ManagerPublicKeys.find(notification.Owner) == pluginConfig.ManagerPublicKeys.end()) {
			return Failure_LiquidityProvider_Invalid_Owner;
		}

		if (notification.SlashingPeriod == 0) {
			return Failure_LiquidityProvider_Invalid_Slashing_Period;
		}

		if (notification.WindowSize < 1 || notification.WindowSize > pluginConfig.MaxWindowSize) {
			return Failure_LiquidityProvider_Invalid_Window_Size;
		}

		if (notification.CurrencyDeposit.unwrap() == 0) {
			return Failure_LiquidityProvider_Invalid_Exchange_Rate;
		}

		if (notification.InitialMosaicsMinting.unwrap() == 0) {
			return Failure_LiquidityProvider_Invalid_Exchange_Rate;
		}

        return ValidationResult::Success;
    })
}}
