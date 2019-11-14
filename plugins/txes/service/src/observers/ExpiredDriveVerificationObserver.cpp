/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "catapult/observers/ObserverUtils.h"
#include "CommonDrive.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(ExpiredDriveVerification, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(ExpiredDriveVerification, Notification, [pConfigHolder](const Notification&, ObserverContext& context) {
			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			const auto& config = pConfigHolder->Config(context.Height);
			auto streamingMosaicId = config.Immutable.StreamingMosaicId;

			driveCache.processEndingDriveVerifications(context.Height, [streamingMosaicId, &context](state::DriveEntry& driveEntry) {
				if ((NotifyMode::Commit == context.Mode && driveEntry.state() != state::DriveState::Verification) ||
					(NotifyMode::Rollback == context.Mode && driveEntry.state() != state::DriveState::InProgress))
					return;

				auto& verificationInfo = driveEntry.verificationHistory().at(context.Height);
				auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
				auto driveAccountIter = accountStateCache.find(driveEntry.key());
				auto& driveState = driveAccountIter.get();
				auto replicatorAccountIter = accountStateCache.find(verificationInfo.Replicator);
				auto& replicatorState = replicatorAccountIter.get();
				if (NotifyMode::Commit == context.Mode) {
					driveState.Balances.debit(streamingMosaicId, verificationInfo.Fee, context.Height);
					replicatorState.Balances.credit(streamingMosaicId, verificationInfo.Fee, context.Height);
					SetDriveState(driveEntry, context, state::DriveState::InProgress);
				} else {
					replicatorState.Balances.debit(streamingMosaicId, verificationInfo.Fee, context.Height);
					driveState.Balances.credit(streamingMosaicId, verificationInfo.Fee, context.Height);
					SetDriveState(driveEntry, context, state::DriveState::Verification);
				}
			});

			if (NotifyMode::Rollback == context.Mode)
				return;

			auto maxRollbackBlocks = config.Network.MaxRollbackBlocks;
			if (context.Height.unwrap() <= maxRollbackBlocks)
				return;

			auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
			driveCache.processEndingDriveVerifications(pruneHeight, [pruneHeight](state::DriveEntry& driveEntry) {
				driveEntry.pruneVerificatioHistory(pruneHeight);
			});
		});
	}
}}
