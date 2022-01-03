/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DataModificationApprovalRefund, model::DataModificationApprovalRefundNotification<1>, [](const model::DataModificationApprovalRefundNotification<1>& notification, ObserverContext& context) {
	  	if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationApprovalRefund)");

	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();

	  	auto& accountStateCache = context.Cache.sub<cache::AccountStateCache>();
	  	auto senderIter = accountStateCache.find(notification.DriveKey);
	  	auto& senderState = senderIter.get();
	  	auto recipientIter = accountStateCache.find(driveEntry.owner());
	  	auto& recipientState = recipientIter.get();

		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
	  	const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;

	  	const auto replicatorDifference = driveEntry.replicatorCount() - driveEntry.replicators().size();
	  	const auto usedSizeDifference = driveEntry.activeDataModifications().begin()->ActualUploadSize
			+ driveEntry.usedSize()
			- (notification.UsedDriveSize - notification.MetaFilesSize);
		const auto transferAmount = Amount(replicatorDifference * usedSizeDifference);

		senderState.Balances.debit(streamingMosaicId, transferAmount, context.Height);
		recipientState.Balances.credit(currencyMosaicId, transferAmount, context.Height);
	});
}}
