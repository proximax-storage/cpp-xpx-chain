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

			auto& automaticExecutionInfo = contractEntry.automaticExecutionsInfo();

			// TODO Prevent overflow
			automaticExecutionInfo.m_automatedExecutionsNumber += notification.Number;

			if (!automaticExecutionInfo.m_automaticExecutionsEnabledSince) {
				if (contractEntry.deploymentStatus() == state::DeploymentStatus::COMPLETED) {
					automaticExecutionInfo.m_automaticExecutionsEnabledSince = context.Height;
				}
			}
		}
	})

}
