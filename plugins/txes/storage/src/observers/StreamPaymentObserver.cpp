/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"

namespace catapult { namespace observers {

	DEFINE_OBSERVER(StreamPayment, model::StreamPaymentNotification<1>, [](const model::StreamPaymentNotification<1>& notification, ObserverContext& context) {
		if (NotifyMode::Rollback == context.Mode)
			CATAPULT_THROW_RUNTIME_ERROR("Invalid observer mode ROLLBACK (StreamPayment)");

	  	auto& driveCache = context.Cache.sub<cache::BcDriveCache>();
	  	auto driveIter = driveCache.find(notification.DriveKey);
	  	auto& driveEntry = driveIter.get();

		auto& activeDataModifications = driveEntry.activeDataModifications();
		auto pModification = std::find_if(
				activeDataModifications.begin(),
				activeDataModifications.end(),
				[notification] (const state::ActiveDataModification& modification) -> bool {
					return notification.StreamId == modification.Id;
				});
		pModification->ExpectedUploadSize += notification.AdditionalUploadSize;
		pModification->ActualUploadSize += notification.AdditionalUploadSize;
	});
}}
