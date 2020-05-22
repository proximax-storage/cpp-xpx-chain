/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/LevyCache.h"
#include "src/model/MosaicLevy.h"

namespace catapult { namespace observers {
		
	using Notification = model::MosaicRemoveLevyNotification<1>;

	void RemoveLevyObserverDetail(
		const Notification& notification,
		const ObserverContext& context) {
		
		auto& cache = context.Cache.sub<cache::LevyCache>();
		auto mosaicId = context.Resolvers.resolve(notification.MosaicId);
		auto iter = cache.find(mosaicId);
		auto& entry = iter.get();
		
		if (NotifyMode::Commit == context.Mode) {
			entry.remove(context.Height);
			cache.markHistoryForRemove(mosaicId, context.Height);
		} else if( NotifyMode::Rollback == context.Mode) {
			entry.undo();
			cache.unmarkHistoryForRemove(mosaicId, context.Height);
		}
	}

	DEFINE_OBSERVER(RemoveLevy, Notification, [](const auto& notification, const ObserverContext& context) {
		RemoveLevyObserverDetail(notification, context);
	});
}}