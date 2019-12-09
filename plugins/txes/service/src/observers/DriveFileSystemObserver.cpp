/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

    using Notification = model::DriveFileSystemNotification<1>;

    DECLARE_OBSERVER(DriveFileSystem, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
        return MAKE_OBSERVER(DriveFileSystem, Notification, [pConfigHolder](const Notification& notification, ObserverContext& context) {
            auto& driveCache = context.Cache.sub<cache::DriveCache>();
            auto driveIter = driveCache.find(notification.DriveKey);
            auto& driveEntry = driveIter.get();

            if (NotifyMode::Commit == context.Mode) {
                driveEntry.setRootHash(notification.RootHash);
            } else {
                driveEntry.setRootHash(notification.RootHash ^ notification.XorRootHash);
            }

            auto& files = driveEntry.files();
            auto addActionsPtr = notification.AddActionsPtr;
            for (auto i = 0u; i < notification.AddActionsCount; ++i, ++addActionsPtr) {
                if (NotifyMode::Commit == context.Mode) {
                    if (!files.count(addActionsPtr->FileHash)) {
                        state::FileInfo info;
                        info.Size = addActionsPtr->FileSize;
                        files.emplace(addActionsPtr->FileHash, info);
                    }

                    for (auto& replicator : driveEntry.replicators())
                        replicator.second.ActiveFilesWithoutDeposit.insert(addActionsPtr->FileHash);
                } else {
                    files.erase(addActionsPtr->FileHash);

                    for (auto& replicator : driveEntry.replicators())
                        replicator.second.ActiveFilesWithoutDeposit.erase(addActionsPtr->FileHash);
                }
            }

            auto removeActionsPtr = notification.RemoveActionsPtr;
            for (auto i = 0u; i < notification.RemoveActionsCount; ++i, ++removeActionsPtr) {
                if (NotifyMode::Commit == context.Mode) {
                    files.erase(removeActionsPtr->FileHash);
                } else {
                    state::FileInfo info;
                    info.Size = removeActionsPtr->FileSize;
                    files.emplace(removeActionsPtr->FileHash, info);
                }
            }

            auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
            auto streamingMosaicId = pConfigHolder->Config(context.Height).Immutable.StreamingMosaicId;
            for (auto& replicatorPair : driveEntry.replicators()) {
                auto accountIter = accountStateCache.find(replicatorPair.first);
                auto& replicatorAccount = accountIter.get();

                removeActionsPtr = notification.RemoveActionsPtr;
                for (auto i = 0u; i < notification.RemoveActionsCount; ++i, ++removeActionsPtr) {
                    if (NotifyMode::Commit == context.Mode) {
                        if (replicatorPair.second.ActiveFilesWithoutDeposit.count(removeActionsPtr->FileHash)) {
                            replicatorPair.second.ActiveFilesWithoutDeposit.erase(removeActionsPtr->FileHash);
                            replicatorPair.second.AddInactiveUndepositedFile(removeActionsPtr->FileHash, context.Height);
                        } else {
                            Credit(replicatorAccount, streamingMosaicId, utils::CalculateFileDeposit(driveEntry, removeActionsPtr->FileHash), context);
                        }
                    } else {
                        if (replicatorPair.second.InactiveFilesWithoutDeposit.count(removeActionsPtr->FileHash)
                            && replicatorPair.second.InactiveFilesWithoutDeposit.at(removeActionsPtr->FileHash).back() == context.Height) {
                            replicatorPair.second.ActiveFilesWithoutDeposit.insert(removeActionsPtr->FileHash);
                            replicatorPair.second.RemoveInactiveUndepositedFile(removeActionsPtr->FileHash, context.Height);
                        } else {
                            Debit(replicatorAccount, streamingMosaicId, utils::CalculateFileDeposit(driveEntry, removeActionsPtr->FileHash), context);
                        }
                    }
                }
            }
	    })
    };
}}
