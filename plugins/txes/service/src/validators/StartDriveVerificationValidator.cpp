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

	using Notification = model::StartDriveVerificationNotification<1>;

	DECLARE_STATEFUL_VALIDATOR(StartDriveVerification, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_STATEFUL_VALIDATOR(StartDriveVerification, [pConfigHolder](const Notification &notification, const ValidatorContext &context) {
			const auto& driveCache = context.Cache.sub<cache::DriveCache>();
			if (!driveCache.contains(notification.DriveKey))
				return Failure_Service_Drive_Does_Not_Exist;

			auto driveIter = driveCache.find(notification.DriveKey);
			const auto &driveEntry = driveIter.get();
			if (driveEntry.state() == state::DriveState::Verification)
				return Failure_Service_Verification_Already_In_Progress;

			if (driveEntry.state() != state::DriveState::InProgress)
				return Failure_Service_Drive_Is_Not_In_Progress;

			if (!driveEntry.replicators().count(notification.Replicator))
				return Failure_Service_Drive_Replicator_Not_Registered;

			const auto& config = pConfigHolder->Config(context.Height);
			const auto& pluginConfig = config.Network.GetPluginConfiguration<config::ServiceConfiguration>(PLUGIN_NAME_HASH(service));
			if (pluginConfig.MinVerificationFee > notification.VerificationFee)
				return Failure_Service_Verification_Fee_Less_Than_Required;

			return ValidationResult::Success;
		});
	};
}}
