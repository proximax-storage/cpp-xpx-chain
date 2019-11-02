/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"
#include "src/cache/DriveCache.h"
#include "src/config/ServiceConfiguration.h"
#include "catapult/validators/ValidatorContext.h"

namespace catapult { namespace validators {

	using Notification = model::DriveFileSystemNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(MaxFilesOnDrive, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(MaxFilesOnDrive, [pConfigHolder](
				const Notification& notification,
				const ValidatorContext& context) {
			const auto& driveCache = context.Cache.sub<cache::DriveCache>();

			auto driveIter = driveCache.find(notification.DriveKey);
			const auto& driveEntry = driveIter.get();
            const model::NetworkConfiguration& networkConfig = pConfigHolder->ConfigAtHeightOrLatest(context.Height).Network;
            const auto& pluginConfig = networkConfig.GetPluginConfiguration<config::ServiceConfiguration>(PLUGIN_NAME_HASH(service));

			if (driveEntry.files().size() + notification.AddActionsCount >= pluginConfig.MaxFilesOnDrive)
				return Failure_Service_Too_Many_Files_On_Drive;

			return ValidationResult::Success;
		});
	}
}}
