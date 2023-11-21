/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DataModificationApprovalRefund, model::DataModificationApprovalRefundNotification<1>, [&liquidityProvider](const model::DataModificationApprovalRefundNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationApprovalRefund)");

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		const auto replicatorDifference = driveEntry.replicatorCount() - driveEntry.replicators().size();
		const auto usedSizeDifference =
				driveEntry.activeDataModifications().begin()->ActualUploadSizeMegabytes
				+ utils::FileSize::FromBytes(driveEntry.usedSizeBytes() - driveEntry.metaFilesSizeBytes()).megabytes()
				- utils::FileSize::FromBytes(notification.UsedDriveSize - notification.MetaFilesSizeBytes).megabytes();
		const auto transferAmount = Amount(replicatorDifference * usedSizeDifference);

		liquidityProvider->debitMosaics(context, driveEntry.key(), driveEntry.owner(),
									   config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
									   transferAmount);

		const auto& modification = *driveEntry.activeDataModifications().begin();
		const auto expectedActualDifference =
				modification.ExpectedUploadSizeMegabytes - modification.ActualUploadSizeMegabytes;
		liquidityProvider->debitMosaics(context, driveEntry.key(), driveEntry.owner(),
										config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
										Amount(driveEntry.replicatorCount() * expectedActualDifference));
	});
}}
