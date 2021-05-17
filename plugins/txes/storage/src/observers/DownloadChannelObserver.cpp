/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(DownloadChannel, model::DownloadNotification<1>, [](const model::DownloadNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (DownloadChannel)");

	  	auto& downloadCache = context.Cache.sub<cache::DownloadChannelCache>();
		state::DownloadChannelEntry downloadEntry(notification.Id);
		downloadEntry.setConsumer(notification.Consumer);
		// TODO: Add according Id to driveEntry
		downloadEntry.setDrive(notification.DriveKey);
		downloadEntry.setTransactionFee(notification.TransactionFee);
		// TODO: Buy storage units for xpx in notification.DownloadSize
		downloadEntry.setStorageUnits(Amount(notification.DownloadSize));
	});
}}
