/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "Validators.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/config/MosaicConfiguration.h"

namespace catapult { namespace validators {

	namespace {
		template<typename TKey>
		ValidationResult CheckAccount(uint16_t maxMosaics, MosaicId mosaicId, const TKey& key, const ValidatorContext& context) {
			const auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
			auto accountStateIter = accountStateCache.find(key);
			if (!accountStateIter.tryGet())
				return ValidationResult::Success;

			const auto& balances = accountStateIter.get().Balances;
			if (balances.get(mosaicId) != Amount())
				return ValidationResult::Success;

			return maxMosaics <= balances.size() ? Failure_Mosaic_Max_Mosaics_Exceeded : ValidationResult::Success;
		}
	}

	DECLARE_STATEFUL_VALIDATOR(MaxMosaicsBalanceTransfer, model::BalanceTransferNotification<1>)() {
		using ValidatorType = stateful::FunctionalNotificationValidatorT<model::BalanceTransferNotification<1>>;
		auto name = "MaxMosaicsBalanceTransferValidator";

		return std::make_unique<ValidatorType>(name, [](const auto& notification, const auto& context) {
			if (Amount() == notification.Amount)
				return ValidationResult::Success;

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::MosaicConfiguration>();
			return CheckAccount(
				pluginConfig.MaxMosaicsPerAccount,
				context.Resolvers.resolve(notification.MosaicId),
				context.Resolvers.resolve(notification.Recipient),
				context);
		});
	}

	DECLARE_STATEFUL_VALIDATOR(MaxMosaicsSupplyChange, model::MosaicSupplyChangeNotification<1>)() {
		using ValidatorType = stateful::FunctionalNotificationValidatorT<model::MosaicSupplyChangeNotification<1>>;
		auto name = "MaxMosaicsSupplyChangeValidator";
		return std::make_unique<ValidatorType>(name, [](const auto& notification, const auto& context) {
			if (model::MosaicSupplyChangeDirection::Decrease == notification.Direction)
				return ValidationResult::Success;

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::MosaicConfiguration>();
			return CheckAccount(pluginConfig.MaxMosaicsPerAccount, context.Resolvers.resolve(notification.MosaicId), notification.Signer, context);
		});
	}
}}
