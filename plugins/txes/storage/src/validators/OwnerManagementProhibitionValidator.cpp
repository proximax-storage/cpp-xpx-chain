/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Validators.h"

namespace catapult { namespace validators {

	using Notification = model::OwnerManagementProhibition<1>;

	DECLARE_STATEFUL_VALIDATOR(OwnerManagementProhibition, Notification)() {
		return MAKE_STATEFUL_VALIDATOR(DeploySupercontract, [](const Notification& notification, const ValidatorContext& context) {
			const auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
            auto driveIter = driveCache.find(notification.DriveKey);
            auto* pDriveEntry = driveIter.tryGet();

            if (!pDriveEntry) {
                return Failure_Storage_Drive_Not_Found;
            }

            auto owner = pDriveEntry->owner();
            if (owner != notification.Signer) {
                return Failure_Storage_Is_Not_Owner;
            }

			const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::StorageConfiguration>();

            auto replicatorSize = pDriveEntry->replicators().size(); 
            if (replicatorSize < 2 * pluginConfig.MinReplicatorCount / 3 + 1) {
                return Failure_Storage_Replicator_Count_Insufficient;
            }

			return ValidationResult::Success;
		})
	}
}}
