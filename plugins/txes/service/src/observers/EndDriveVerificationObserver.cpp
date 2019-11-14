/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

	using Notification = model::EndDriveVerificationNotification<1>;

	DECLARE_OBSERVER(EndDriveVerification, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(EndDriveVerification, Notification, [pConfigHolder](const Notification& notification, ObserverContext& context) {
			auto streamingMosaicId = pConfigHolder->Config(context.Height).Immutable.StreamingMosaicId;

			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			auto& driveEntry = driveIter.get();

			auto newDriveState = (NotifyMode::Commit == context.Mode) ? state::DriveState::InProgress : state::DriveState::Verification;
			SetDriveState(driveEntry, context, newDriveState);
			UpdateDriveMultisigSettings(driveEntry, context);

			auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
			auto driveAccountIter = accountStateCache.find(driveEntry.key());
			auto& driveState = driveAccountIter.get();

			auto replicatorCount = driveEntry.replicators().size();
			auto& verificationInfo = driveEntry.verificationHistory().lower_bound(context.Height)->second;
			auto fee = Amount(verificationInfo.Fee.unwrap() / replicatorCount);
			auto totalFee = Amount(fee.unwrap() * replicatorCount);

			if (NotifyMode::Commit == context.Mode)
				driveState.Balances.debit(streamingMosaicId, totalFee, context.Height);
			else
				driveState.Balances.credit(streamingMosaicId, totalFee, context.Height);

			for (const auto& pair : driveEntry.replicators()) {
				auto replicatorAccountIter = accountStateCache.find(pair.first);
				auto &replicatorState = replicatorAccountIter.get();
				if (NotifyMode::Commit == context.Mode)
					replicatorState.Balances.credit(streamingMosaicId, fee, context.Height);
				else
					replicatorState.Balances.debit(streamingMosaicId, fee, context.Height);
			}
		});
	}
}}
