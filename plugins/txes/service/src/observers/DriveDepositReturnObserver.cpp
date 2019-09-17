/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/model/NetworkConfiguration.h"
#include "catapult/plugins/PluginUtils.h"
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
#include "src/cache/DriveCache.h"
#include <math.h>

namespace catapult { namespace observers {

	DECLARE_OBSERVER(DriveDepositReturn, model::DriveDepositReturnNotification<1>)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(DriveDeposit, model::DriveDepositNotification<1>, [pConfigHolder](const auto& notification, const ObserverContext& context) {
			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			auto& driveEntry = driveCache.find(notification.Drive).get();
			auto& replicatorDepositMap = driveEntry.replicators()[notification.Replicator];
			if (NotifyMode::Commit == context.Mode) {
				replicatorDepositMap.erase(Hash256());
			} else {
				replicatorDepositMap.emplace(Hash256(), notification.Deposit);
			}

			auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
			auto& multisigEntry = multisigCache.find(notification.Drive).get();
			float cosignatoryCount = multisigEntry.cosignatories().size();
			const model::NetworkConfiguration& networkConfig = pConfigHolder->Config(context.Height).Network;
			const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::ServiceConfiguration>(PLUGIN_NAME_HASH(service));
			multisigEntry.setMinApproval(ceil(cosignatoryCount * pluginConfig.MinPercentageOfApproval / 100));
			multisigEntry.setMinRemoval(ceil(cosignatoryCount * pluginConfig.MinPercentageOfRemoval / 100));
		});
	}
}}
