/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/LevyCache.h"

namespace catapult { namespace observers {

	using Notification = model::MosaicModifyLevyNotification<1>;

	void ModifyLevyObserverDetail(
			const Notification& notification,
			const ObserverContext& context) {

		auto mosaicId = context.Resolvers.resolve(notification.MosaicId);
		MosaicId levyMosaicId = context.Resolvers.resolve(notification.Levy.MosaicId);

		Address address = context.Resolvers.resolve(notification.Levy.Recipient);
		
		auto& cache = context.Cache.sub<cache::LevyCache>();
		auto iter = cache.find(mosaicId);
		
		auto pLevy = std::make_unique<state::LevyEntryData>(notification.Levy.Type,
			address, levyMosaicId, notification.Levy.Fee);
	
		if (NotifyMode::Commit == context.Mode) {
			if(iter.tryGet()) {
				auto& entry = iter.get();
				entry.update(*pLevy, context.Height);
				cache.markHistoryForRemove(mosaicId, context.Height);
				
			}else{
				cache.insert(state::LevyEntry(mosaicId, std::move(pLevy)));
			}
		}
		else if (NotifyMode::Rollback == context.Mode) {
			auto& entry = iter.get();
			if(entry.hasUpdateHistory()) {
				entry.undo();
				cache.unmarkHistoryForRemove(mosaicId, context.Height);
			}
			else {
				cache.remove(mosaicId);
			}
		}
	}

	DEFINE_OBSERVER(ModifyLevy, Notification, [](const auto& notification, const ObserverContext& context) {
		ModifyLevyObserverDetail(notification, context);
	});
}}