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

namespace catapult { namespace validators {

	using Notification = model::HashLockMosaicNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(HashLockMosaic, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(HashLockMosaic, ([](const auto& notification, const auto& context) {
			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::HashLockConfiguration>();
			if (pluginConfig.LockedFundsPerAggregate != notification.Mosaic.Amount)
				return Failure_LockHash_Invalid_Mosaic_Amount;

			auto currencyMosaicId = context.Resolvers.resolve(config::GetUnresolvedCurrencyMosaicId(context.Config.Immutable));
			return currencyMosaicId != context.Resolvers.resolve(notification.Mosaic.MosaicId)
					? Failure_LockHash_Invalid_Mosaic_Id
					: ValidationResult::Success;
		}));
	}
}}

