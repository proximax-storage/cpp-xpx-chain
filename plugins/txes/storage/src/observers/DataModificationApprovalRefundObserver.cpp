/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	void DataModificationApprovalRefundHandler(const LiquidityProviderExchangeObserver& liquidityProvider, const model::DataModificationApprovalRefundNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationApprovalRefund)");

		CATAPULT_LOG(error) << "liquidityProvider address " << &liquidityProvider;

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

		const auto replicatorDifference = driveEntry.replicatorCount() - driveEntry.replicators().size();
		const auto usedSizeDifference =
				driveEntry.activeDataModifications().begin()->ExpectedUploadSizeMegabytes
				+ utils::FileSize::FromBytes(driveEntry.usedSizeBytes() - driveEntry.metaFilesSizeBytes()).megabytes()
				- utils::FileSize::FromBytes(notification.UsedDriveSize - notification.MetaFilesSizeBytes).megabytes();
		const auto transferAmount = Amount(replicatorDifference * usedSizeDifference);
		const auto configValue = config::GetUnresolvedStreamingMosaicId(context.Config.Immutable);

		liquidityProvider.debitMosaics(context, driveEntry.key(), driveEntry.owner(),
									   configValue,
									   transferAmount);
	}

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DataModificationApprovalRefund, model::DataModificationApprovalRefundNotification<1>, [&liquidityProvider](const model::DataModificationApprovalRefundNotification<1>& notification, ObserverContext& context) {
		DataModificationApprovalRefundHandler(liquidityProvider, notification, context);
	});
}}
