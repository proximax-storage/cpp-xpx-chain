/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"
#include "catapult/plugins/PluginUtils.h"

namespace catapult { namespace observers {

    using Notification = model::StartDriveVerificationNotification<1>;

    DECLARE_OBSERVER(StartDriveVerification, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
        return MAKE_OBSERVER(StartDriveVerification, Notification, [pConfigHolder](const Notification& notification, ObserverContext& context) {
			const auto& config = pConfigHolder->ConfigAtHeightOrLatest(context.Height);
			const auto& pluginConfig = config.Network.GetPluginConfiguration<config::ServiceConfiguration>(PLUGIN_NAME_HASH(service));
			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();
			auto verificationEnd = Height(context.Height.unwrap() + pluginConfig.VerificationDuration.unwrap());
			if (NotifyMode::Commit == context.Mode) {
				driveEntry.verificationHistory().emplace(verificationEnd, state::VerificationInfo{notification.Replicator, notification.VerificationFee});
				SetDriveState(driveEntry, context, state::DriveState::Verification);
				driveCache.setDriveVerificationEnd(notification.DriveKey, verificationEnd);
			} else {
				driveEntry.verificationHistory().erase(verificationEnd);
				SetDriveState(driveEntry, context, state::DriveState::InProgress);
				driveCache.unsetDriveVerificationEnd(notification.DriveKey, verificationEnd);
			}
        });
    }
}}
