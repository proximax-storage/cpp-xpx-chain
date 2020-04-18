/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/LevyCache.h"
#include "src/model/MosaicLevy.h"
#include "src/model/MosaicModifyLevyTransaction.h"

namespace catapult { namespace observers {

	using Notification = model::MosaicUpdateLevyNotification<1>;

	void UpdateLevyObserverDetail(
			const Notification& notification,
			const ObserverContext& context) {
		
		auto& levyCache = context.Cache.sub<cache::LevyCache>();
		auto mosaicIter = levyCache.find(notification.MosaicId);
		
		auto& entry = mosaicIter.get();
		auto& levy = entry.levyRef();
		
		if (NotifyMode::Commit == context.Mode) {
			if( notification.UpdateFlag & model::MosaicLevyModifyBitChangeType)
				levy.Type = notification.Levy.Type;
			
			if( notification.UpdateFlag & model::MosaicLevyModifyBitChangeRecipient)
				levy.Recipient = notification.Levy.Recipient;
			
			if( notification.UpdateFlag & model::MosaicLevyModifyBitChangeMosaicId)
				levy.MosaicId = notification.Levy.MosaicId;
			
			if( notification.UpdateFlag & model::MosaicLevyModifyBitChangeLevyFee)
				levy.Fee = notification.Levy.Fee;
		}
	}

	DEFINE_OBSERVER(UpdateLevy, Notification, [](const auto& notification, const ObserverContext& context) {
		UpdateLevyObserverDetail(notification, context);
	});
}}