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

	DECLARE_STATEFUL_VALIDATOR(BlockchainUpgrade, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(BlockchainUpgrade, ([pConfigHolder](const Notification& notification, const ValidatorContext& context) {
			if (notification.Version <= version::BlockchainVersion)
				return Failure_BlockchainUpgrade_Version_Lower_Than_Current;

			auto upgradePeriod = notification.UpgradePeriod.unwrap();
			const model::NetworkConfiguration& networkConfig = pConfigHolder->Config(context.Height).Network;
			const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::BlockchainUpgradeConfiguration>(PLUGIN_NAME_HASH(upgrade));
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
