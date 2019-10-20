/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/BuyOfferCache.h"
#include "src/cache/DealCache.h"
#include "src/cache/OfferDeadlineCache.h"
#include "src/cache/SellOfferCache.h"

namespace catapult { namespace observers {

	using Notification = model::BlockNotification<1>;

	template<typename TDeadlineMap, typename THeightMap>
	void MarkOffersForRemove(
			TDeadlineMap& deadlines,
			THeightMap& heights,
			const Timestamp& timestamp,
			const Height& currentHeight) {
		auto endIter = deadlines.upper_bound(timestamp);
		for (auto deadlineIter = deadlines.begin(); deadlineIter != endIter;) {
			auto range = heights.equal_range(currentHeight);
			bool found = false;
			for (auto heightIter = range.first; heightIter != range.second; ++heightIter) {
				if (heightIter->second == deadlineIter->second) {
					found = true;
					break;
				}
			}
			if (!found) {
				heights.emplace(currentHeight, deadlineIter->second);
			}
		}
	}

	template<typename TCache, typename THeightMap>
	void CleanupOffers(
			TCache& cache,
			THeightMap& heights,
			const Height& pruneHeight,
			cache::DealCacheDelta& dealCache,
			state::OfferDeadlineEntry::OfferDeadlineMap& deadlines) {
		// TODO: give back locked funds.
		if (heights.size()) {
			auto endIter = heights.upper_bound(pruneHeight);
			for (auto iter = heights.begin(); iter != endIter;) {
				for (auto deadlineIter = deadlines.begin(); deadlineIter != deadlines.end(); ++deadlineIter) {
					if (deadlineIter->second == iter->second) {
						deadlines.erase(deadlineIter);
						break;
					}
				}
				if (cache.contains(iter->second))
					cache.remove(iter->second);
				if (dealCache.contains(iter->second))
					dealCache.remove(iter->second);
				iter = heights.erase(iter);
			}
		}
	}

	DECLARE_OBSERVER(CleanupOffers, Notification)(plugins::PluginManager& manager) {
		return MAKE_OBSERVER(CleanupOffers, Notification, [&manager](const auto& notification, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				return;

			auto& buyOfferCache = context.Cache.sub<cache::BuyOfferCache>();
			auto& sellOfferCache = context.Cache.sub<cache::SellOfferCache>();
			auto& dealCache = context.Cache.sub<cache::DealCache>();
			auto& offerDeadlineCache = context.Cache.sub<cache::OfferDeadlineCache>();
			if (!offerDeadlineCache.contains(Height(0)))
				offerDeadlineCache.insert(state::OfferDeadlineEntry(Height(0)));
			state::OfferDeadlineEntry& offerDeadlineEntry = offerDeadlineCache.find(Height(0)).get();

			MarkOffersForRemove(offerDeadlineEntry.buyOfferDeadlines(), offerDeadlineEntry.buyOfferHeights(), notification.Timestamp, context.Height);
			MarkOffersForRemove(offerDeadlineEntry.sellOfferDeadlines(), offerDeadlineEntry.sellOfferHeights(), notification.Timestamp, context.Height);

			auto maxRollbackBlocks = manager.configHolder()->Config(context.Height).Network.MaxRollbackBlocks;
			if (context.Height.unwrap() > maxRollbackBlocks) {
				auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
				CleanupOffers(buyOfferCache, offerDeadlineEntry.buyOfferHeights(), pruneHeight, dealCache, offerDeadlineEntry.buyOfferDeadlines());
				CleanupOffers(sellOfferCache, offerDeadlineEntry.sellOfferHeights(), pruneHeight, dealCache, offerDeadlineEntry.sellOfferDeadlines());
			}
		})
	}
}}
