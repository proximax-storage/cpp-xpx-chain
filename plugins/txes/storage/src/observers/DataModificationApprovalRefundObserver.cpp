/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	using Notification = model::DataModificationApprovalRefundNotification<1>;

	DEFINE_OBSERVER_WITH_LIQUIDITY_PROVIDER(DataModificationApprovalRefund, Notification, [&liquidityProvider](const Notification& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DataModificationApprovalRefund)");

		auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
		auto driveIter = driveCache.find(notification.DriveKey);
		auto& driveEntry = driveIter.get();

	  	const auto& currencyMosaicId = context.Config.Immutable.CurrencyMosaicId;
		const auto& streamingMosaicId = context.Config.Immutable.StreamingMosaicId;
		auto& statementBuilder = context.StatementBuilder();

		const auto replicatorDifference = driveEntry.replicatorCount() - driveEntry.replicators().size();
		const auto usedSizeDifference =
				driveEntry.activeDataModifications().begin()->ActualUploadSizeMegabytes
				+ utils::FileSize::FromBytes(driveEntry.usedSizeBytes() - driveEntry.metaFilesSizeBytes()).megabytes()
				- utils::FileSize::FromBytes(notification.UsedDriveSize - notification.MetaFilesSizeBytes).megabytes();
		const auto transferAmount = Amount(replicatorDifference * usedSizeDifference);

		if (transferAmount.unwrap() > 0) {
			liquidityProvider->debitMosaics(
					context,
					driveEntry.key(),
					driveEntry.owner(),
					config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
					transferAmount);

			// Adding Refund receipt.
			const auto receiptType = model::Receipt_Type_Data_Modification_Approval_Refund;
			const model::StorageReceipt receipt(
					receiptType,
					driveEntry.key(),
					driveEntry.owner(),
					{ streamingMosaicId, currencyMosaicId },
					transferAmount);
			statementBuilder.addTransactionReceipt(receipt);
		}

		const auto& modification = *driveEntry.activeDataModifications().begin();
		const auto expectedActualDifference =
				modification.ExpectedUploadSizeMegabytes - modification.ActualUploadSizeMegabytes;
	  	const auto streamTransferAmount = Amount(driveEntry.replicatorCount() * expectedActualDifference);

		if (streamTransferAmount.unwrap() > 0) {
			liquidityProvider->debitMosaics(
					context,
					driveEntry.key(),
					driveEntry.owner(),
					config::GetUnresolvedStreamingMosaicId(context.Config.Immutable),
					streamTransferAmount);

			// Adding Refund Stream receipt.
			const auto receiptType = model::Receipt_Type_Data_Modification_Approval_Refund_Stream;
			const model::StorageReceipt receipt(
					receiptType,
					driveEntry.key(),
					driveEntry.owner(),
					{ streamingMosaicId, currencyMosaicId },
					streamTransferAmount);
			statementBuilder.addTransactionReceipt(receipt);
		}
	});
}}
