/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/model/NetworkConfiguration.h"
#include "src/cache/DriveCache.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(FilesDeposit, model::FilesDepositNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		state::ReplicatorInfo& replicator = driveEntry.replicators().at(notification.Replicator);

		auto filesPtr = notification.FilesPtr;
		for (auto i = 0u; i < notification.FilesCount; ++i, ++filesPtr) {
			if (NotifyMode::Commit == context.Mode) {
                replicator.ActiveFilesWithoutDeposit.erase(filesPtr->FileHash);
			} else {
                replicator.ActiveFilesWithoutDeposit.insert(filesPtr->FileHash);
			}
		}
	});
}}
