/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include <cmath>
#include "Observers.h"
#include "src/cache/DriveCache.h"
#include "plugins/txes/multisig/src/cache/MultisigCache.h"
#include "src/utils/ServiceUtils.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(JoinToDrive, model::JoinToDriveNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto& driveEntry = driveCache.find(notification.DriveKey).get();
		if (NotifyMode::Commit == context.Mode) {
			state::ReplicatorInfo info;
			info.Start = context.Height;
			info.Deposit = utils::CalculateDriveDeposit(driveEntry);

			// It is new replicator, so he doesn't have any files
			for (const auto& file : driveEntry.files())
				if (file.second.isActive())
					info.AddFile(file.first);

			driveEntry.replicators().emplace(notification.Replicator, info);

			if (driveEntry.replicators().size() >= driveEntry.minReplicators() && driveEntry.billingHistory().empty())
                driveEntry.setState(state::DriveState::Pending);
		} else {
			driveEntry.replicators().erase(notification.Replicator);
            if (driveEntry.replicators().size() < driveEntry.minReplicators() && driveEntry.billingHistory().empty())
                driveEntry.setState(state::DriveState::NotStarted);
		}

        auto& multisigCache = context.Cache.sub<cache::MultisigCache>();
        auto& multisigEntry = multisigCache.find(notification.DriveKey).get();
        float cosignatoryCount = driveEntry.replicators().size() + 1;
        multisigEntry.setMinApproval(ceil(cosignatoryCount * driveEntry.minApprovers() / 100));
        multisigEntry.setMinRemoval(ceil(cosignatoryCount * driveEntry.minApprovers() / 100));
	});
}}
