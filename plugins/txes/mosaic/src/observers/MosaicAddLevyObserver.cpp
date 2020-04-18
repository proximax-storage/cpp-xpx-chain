/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/LevyCache.h"
#include "src/model/MosaicLevy.h"

namespace catapult { namespace observers {

	using Notification = model::MosaicAddLevyNotification<1>;

	void AddLevyObserverDetail(
			const Notification& notification,
			const ObserverContext& context) {
		
		auto& cache = context.Cache.sub<cache::LevyCache>();
		if (NotifyMode::Commit == context.Mode) {
			cache.insert(state::LevyEntry(notification.MosaicId, notification.Levy));
		} else {
			if (cache.contains(notification.MosaicId))
				cache.remove(notification.MosaicId);
		}
	}

	DEFINE_OBSERVER(AddLevy, Notification, [](const auto& notification, const ObserverContext& context) {
		AddLevyObserverDetail(notification, context);
	});
}}