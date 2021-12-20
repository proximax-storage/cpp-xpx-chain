/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DataModificationCancel,model::DataModificationCancelNotification<1>,[](const model::DataModificationCancelNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationCancel)");

	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto& driveEntry = driveCache.find(notification.DriveKey).get();

		auto& activeDataModifications = driveEntry.activeDataModifications();
		auto activeDataModificationIter = activeDataModifications.begin();

		for (; activeDataModificationIter != activeDataModifications.end(); ++activeDataModificationIter) {
			if (activeDataModificationIter->Id == notification.DataModificationId) {
				activeDataModifications.erase(activeDataModificationIter);
				driveEntry.completedDataModifications().emplace_back(state::CompletedDataModification{
						*activeDataModificationIter,
						state::DataModificationState::Cancelled
				});
				break;
			}
		}

		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
		auto driveOwnerIter = accountStateCache.find(driveEntry.owner());
		auto& driveOwner = driveOwnerIter.get();

		if (activeDataModificationIter->Id != activeDataModifications.front().Id) {
			const auto refund = Amount(2 * activeDataModificationIter->ExpectedUploadSize * driveEntry.replicatorCount());
			driveOwner.Balances.debit(streamingMosaicId, refund, context.Height);

			return;
		}

		const auto downloadRefund = Amount(activeDataModificationIter->ExpectedUploadSize);
		const auto uploadRefund = Amount(activeDataModificationIter->ExpectedUploadSize
										 * (driveEntry.replicators().size() - 1) / driveEntry.replicators().size());

		for (const auto& replicator: driveEntry.replicators()) {
			auto replicatorIter = accountStateCache.find(replicator);
			auto& rep = replicatorIter.get();

			rep.Balances.debit(streamingMosaicId, downloadRefund, context.Height);
			rep.Balances.debit(streamingMosaicId, uploadRefund, context.Height);
		}

		const auto clientUploadRefund = Amount(activeDataModificationIter->ExpectedUploadSize);
		const auto streamingRefundAmount = Amount(2 * activeDataModificationIter->ExpectedUploadSize *
											   (driveEntry.replicatorCount() - driveEntry.replicators().size()));
		driveOwner.Balances.debit(streamingMosaicId, clientUploadRefund, context.Height);
		driveOwner.Balances.debit(streamingMosaicId, streamingRefundAmount, context.Height);
	})
}}
