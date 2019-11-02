/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/DriveCache.h"
#include "src/utils/ServiceUtils.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DriveFileSystem, model::DriveFileSystemNotification<1>, [](const auto& notification, const ObserverContext& context) {
		auto& driveCache = context.Cache.sub<cache::DriveCache>();
        auto driveIter = driveCache.find(notification.DriveKey);
        auto& driveEntry = driveIter.get();

		if (NotifyMode::Commit == context.Mode) {
            driveEntry.setRootHash(notification.RootHash);
		} else {
            driveEntry.setRootHash(notification.RootHash ^ notification.XorRootHash);
		}

		auto addActionsPtr = notification.AddActionsPtr;
		for (auto i = 0u; i < notification.AddActionsCount; ++i, ++addActionsPtr) {
            if (NotifyMode::Commit == context.Mode) {
                auto& files = driveEntry.files();
                if (!files.count(addActionsPtr->FileHash)) {
                    state::FileInfo info;
                    info.Size = addActionsPtr->FileSize;
                    files.emplace(addActionsPtr->FileHash, info);
                }

                auto& info = files[addActionsPtr->FileHash];
                info.Actions.emplace_back(state::DriveAction{ state::DriveActionType::Add, context.Height });
                info.Deposit = info.Deposit + utils::CalculateFileUpload(driveEntry, addActionsPtr->FileSize);

                for (auto& replicator : driveEntry.replicators())
                    replicator.second.IncrementUndepositedFileCounter(addActionsPtr->FileHash);
            } else {
                auto& files = driveEntry.files();
                auto& info = files[addActionsPtr->FileHash];
                info.Deposit = info.Deposit - utils::CalculateFileUpload(driveEntry, addActionsPtr->FileSize);

                if (info.Actions.back().ActionHeight != context.Height || info.Actions.back().Type != state::DriveActionType::Add)
                    CATAPULT_THROW_RUNTIME_ERROR("Got unexpected state during rollback added file");

                info.Actions.pop_back();
                if (info.Actions.empty()) {
                    files.erase(addActionsPtr->FileHash);
                }

                for (auto& replicator : driveEntry.replicators())
                    replicator.second.DecrementUndepositedFileCounter(addActionsPtr->FileHash);
            }
		}

        auto removeActionsPtr = notification.RemoveActionsPtr;
        for (auto i = 0u; i < notification.RemoveActionsCount; ++i, ++removeActionsPtr) {
            if (NotifyMode::Commit == context.Mode) {
                auto& files = driveEntry.files();
                auto& info = files[removeActionsPtr->FileHash];
                info.Actions.emplace_back(state::DriveAction{ state::DriveActionType::Remove, context.Height });
            } else {
                auto& files = driveEntry.files();
                auto& info = files[removeActionsPtr->FileHash];

                if (info.Actions.back().ActionHeight != context.Height || info.Actions.back().Type != state::DriveActionType::Remove)
                    CATAPULT_THROW_RUNTIME_ERROR("Got unexpected state during rollback removed file");

                info.Actions.pop_back();
            }
        }
	});
}}
