/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/version/version.h"
#include "src/cache/BlockchainUpgradeCache.h"
#include "src/config/BlockchainUpgradeConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::BlockchainUpgradeVersionNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(BlockchainUpgrade, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(BlockchainUpgrade, ([](const Notification& notification, const ValidatorContext& context) {
			auto upgradePeriod = notification.UpgradePeriod.unwrap();
            const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::BlockchainUpgradeConfiguration>();
			if (pluginConfig.MinUpgradePeriod.unwrap() > upgradePeriod)
				return Failure_BlockchainUpgrade_Upgrade_Period_Too_Low;

			const auto& cache = context.Cache.sub<cache::BlockchainUpgradeCache>();
			auto height = Height{context.Height.unwrap() + upgradePeriod};
			if (cache.find(height).tryGet())
				return Failure_BlockchainUpgrade_Redundant;

			return ValidationResult::Success;
		}));
	}
}}
