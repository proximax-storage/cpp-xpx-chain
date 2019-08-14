/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "catapult/validators/ValidatorContext.h"
#include "catapult/version/version.h"
#include "src/cache/CatapultUpgradeCache.h"
#include "src/config/CatapultUpgradeConfiguration.h"

namespace catapult { namespace validators {

	using Notification = model::CatapultUpgradeVersionNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(CatapultUpgrade, Notification)(const std::shared_ptr<config::LocalNodeConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(CatapultUpgrade, ([pConfigHolder](const Notification& notification, const ValidatorContext& context) {
			if (notification.Version <= version::CatapultVersion)
				return Failure_CatapultUpgrade_Catapult_Version_Lower_Than_Current;

			auto upgradePeriod = notification.UpgradePeriod.unwrap();
			const model::BlockChainConfiguration& blockChainConfig = pConfigHolder->Config(context.Height).BlockChain;
			const auto& pluginConfig = blockChainConfig.GetPluginConfiguration<config::CatapultUpgradeConfiguration>(PLUGIN_NAME_HASH(upgrade));
			if (pluginConfig.MinUpgradePeriod.unwrap() > upgradePeriod)
				return Failure_CatapultUpgrade_Upgrade_Period_Too_Low;

			const auto& cache = context.Cache.sub<cache::CatapultUpgradeCache>();
			auto height = Height{context.Height.unwrap() + upgradePeriod};
			if (cache.find(height).tryGet())
				return Failure_CatapultUpgrade_Redundant;

			return ValidationResult::Success;
		}));
	}
}}
