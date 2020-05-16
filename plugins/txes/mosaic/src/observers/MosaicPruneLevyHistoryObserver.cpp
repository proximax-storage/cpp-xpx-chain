/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/LevyCache.h"

namespace catapult { namespace observers {
		
	using Notification = model::BlockNotification<1>;
	
	namespace {
		void PruneHistory(state::LevyHistoryIterator& iterator,
			state::LevyHistoryList& list, const Height& height) {
			
			while( iterator != list.end()) {
				if( iterator->first == height)
					iterator = list.erase(iterator);
				else
					iterator++;
			}
		}
	}
	
	void PruneLevyHistoryObserverDetail(const ObserverContext& context) {
		
		if (NotifyMode::Rollback == context.Mode)
			return;
		
		const model::NetworkConfiguration &config = context.Config.Network;
		
		auto gracePeriod = Height(config.MaxRollbackBlocks);
		if (context.Height <= gracePeriod)
			return;
		
		auto pruneHeight = context.Height - gracePeriod;
		auto &cache = context.Cache.sub<cache::LevyCache>();
		
		auto cachedMosaicIds = cache.getCachedMosaicIdsByHeight(pruneHeight);
		for (const auto& mosaicId : cachedMosaicIds) {
			auto cacheIter = cache.find(mosaicId);
			auto &entry = cacheIter.get();
			
			auto iterator = entry.historyAtHeight(pruneHeight);
			PruneHistory(iterator, entry.updateHistory(), pruneHeight);
			
			if( entry.levy() == nullptr && !entry.hasUpdateHistory())
				cache.remove(mosaicId);
		}
	}

	DEFINE_OBSERVER(PruneLevyHistory, Notification, [](const auto&, const ObserverContext& context) {
		PruneLevyHistoryObserverDetail(context);
	});
}}