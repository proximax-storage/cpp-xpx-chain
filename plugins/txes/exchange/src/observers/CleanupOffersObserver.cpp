/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace observers {

	namespace {
		template<typename TExpiredOfferMap>
		void RemoveExpiredOffers(TExpiredOfferMap& expiredOffers, const Height& height) {
			if (expiredOffers.count(height)) {
				expiredOffers.at(height).clear();
				expiredOffers.erase(height);
			}
		}
	}

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(CleanupOffers, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(CleanupOffers, Notification, ([pConfigHolder](const Notification&, const ObserverContext& context) {
			const auto& config = pConfigHolder->Config(context.Height);
			auto currencyMosaicId = config.Immutable.CurrencyMosaicId;
			auto& cache = context.Cache.sub<cache::ExchangeCache>();
			auto expiringOfferOwners = cache.expiringOfferOwners(context.Height);
			for (const auto& key : expiringOfferOwners) {
				auto cacheIter = cache.find(key);
				auto& entry = cacheIter.get();
				OfferExpiryUpdater offerExpiryUpdater(cache, entry);

				Amount xpxAmount(0);
				auto onBuyOfferExpired = [&xpxAmount](const state::BuyOfferMap::const_iterator& iter) {
					xpxAmount = xpxAmount + iter->second.ResidualCost;
				};
				auto onSellOfferExpired = [&entry, currencyMosaicId, &context](const state::SellOfferMap::const_iterator& iter) {
					CreditAccount(entry.owner(), iter->first, iter->second.Amount, context);
				};

				if (NotifyMode::Commit == context.Mode) {
					entry.expireOffers(context.Height, onBuyOfferExpired, onSellOfferExpired);
				} else {
					entry.unexpireOffers(context.Height, onBuyOfferExpired, onSellOfferExpired);
				}

				CreditAccount(entry.owner(), currencyMosaicId, xpxAmount, context);
			}

			if (NotifyMode::Rollback == context.Mode)
				return;

			auto maxRollbackBlocks = config.Network.MaxRollbackBlocks;
			if (context.Height.unwrap() <= maxRollbackBlocks)
				return;

			auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
			expiringOfferOwners = cache.expiringOfferOwners(pruneHeight);
			for (const auto& key : expiringOfferOwners) {
				auto cacheIter = cache.find(key);
				auto& entry = cacheIter.get();
				OfferExpiryUpdater offerExpiryUpdater(cache, entry);
				RemoveExpiredOffers(entry.expiredBuyOffers(), pruneHeight);
				RemoveExpiredOffers(entry.expiredSellOffers(), pruneHeight);
			}
		}))
	}
}}
