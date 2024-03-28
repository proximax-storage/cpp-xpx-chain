/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"

namespace catapult::observers {

	using Notification = model::AutomaticExecutionsReplenishmentNotification<1>;

	DEFINE_OBSERVER(AutomaticExecutionsReplenishment, Notification, [](const Notification & notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (AutomaticExecutionsReplenishment)");

		if (notification.Number > 0U) {
			auto& contractCache = context.Cache.sub<cache::SuperContractCache>();
			auto contractIt = contractCache.find(notification.ContractKey);
			auto& contractEntry = contractIt.get();

			auto& automaticExecutionsInfo = contractEntry.automaticExecutionsInfo();

			automaticExecutionsInfo.AutomatedExecutionsNumber += notification.Number;

			if (!automaticExecutionsInfo.AutomaticExecutionsPrepaidSince) {
				if (contractEntry.deploymentStatus() == state::DeploymentStatus::COMPLETED) {
					automaticExecutionsInfo.AutomaticExecutionsPrepaidSince = context.Height;
				}
			}
		}
	})

}
