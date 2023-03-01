/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <random>
#include "Observers.h"
#include <catapult/cache/ReadOnlyCatapultCache.h>

namespace catapult::observers {

	using Notification = model::DeploySupercontractNotification<1>;

	DECLARE_OBSERVER(DeployContract, Notification)(const std::unique_ptr<state::DriveStateBrowser>& driveBrowser) {
		return MAKE_OBSERVER(
				EndBatchExecution,
				Notification,
				([&driveBrowser](const Notification& notification, ObserverContext& context) {
					if (NotifyMode::Rollback == context.Mode)
						CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DeployContract)");

					auto& contractCache = context.Cache.sub<cache::SuperContractCache>();

					state::SuperContractEntry entry(notification.ContractKey);
					entry.setDriveKey(notification.DriveKey);
					entry.setExecutionPaymentKey(notification.ExecutionPaymentKey);
					entry.setAssignee(notification.Assignee);
					entry.setCreator(notification.Signer);

					auto readOnlyCache = context.Cache.toReadOnly();
					entry.setDeploymentBaseModificationId(driveBrowser->getLastModificationId(readOnlyCache, notification.DriveKey));

					auto& automaticExecutionsInfo = entry.automaticExecutionsInfo();
					automaticExecutionsInfo.AutomaticExecutionFileName = notification.AutomaticExecutionFileName;
					automaticExecutionsInfo.AutomaticExecutionsFunctionName =
							notification.AutomaticExecutionsFunctionName;
					automaticExecutionsInfo.AutomaticExecutionCallPayment = notification.AutomaticExecutionCallPayment;
					automaticExecutionsInfo.AutomaticDownloadCallPayment = notification.AutomaticDownloadCallPayment;

					contractCache.insert(entry);

					auto& driveCache = context.Cache.sub<cache::DriveContractCache>();

					state::DriveContractEntry driveContractEntry(notification.DriveKey);
					driveContractEntry.setContractKey(notification.ContractKey);

					driveCache.insert(driveContractEntry);
				}))
	}
} // namespace catapult::observers
