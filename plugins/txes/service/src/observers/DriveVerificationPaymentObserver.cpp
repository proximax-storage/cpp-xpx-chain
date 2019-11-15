/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "CommonDrive.h"

namespace catapult { namespace observers {

	using Notification = model::DriveVerificationPaymentNotification<1>;

	DECLARE_OBSERVER(DriveVerificationPayment, Notification)(const MosaicId& storageMosaicId) {
		return MAKE_OBSERVER(DriveVerificationPayment, Notification, [storageMosaicId](const auto& notification, ObserverContext& context) {
			auto& driveCache = context.Cache.sub<cache::DriveCache>();
			auto driveIter = driveCache.find(notification.DriveKey);
			state::DriveEntry& driveEntry = driveIter.get();

			auto pFailure = notification.FailuresPtr;
			std::vector<Key> faultyReplicatorKeys(notification.FailureCount);
			for (auto i = 0u; i < notification.FailureCount; ++i, ++pFailure) {
				faultyReplicatorKeys.emplace_back(pFailure->Replicator);
				driveEntry.replicators().at(pFailure->Replicator).End = context.Height;
			}

			DrivePayment(driveEntry, context, storageMosaicId, faultyReplicatorKeys);
			UpdateDriveMultisigSettings(driveEntry, context);
		})
	}
}}
