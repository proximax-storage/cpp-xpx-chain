/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace observers {

	void CreditAccount(const Key& recipient, const MosaicId& mosaicId, const Amount& amount, const ObserverContext& context) {
		if (Amount(0) == amount)
			return;

		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto iter = cache.find(recipient);
		auto& recipientState = iter.get();
		recipientState.Balances.credit(mosaicId, amount, context.Height);
	}

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(CleanupOffers, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(CleanupOffers, Notification, ([pConfigHolder](const Notification&, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::ExchangeCache>();
			auto expiringOfferKeys = cache.touch(context.Height);
			for (const auto& key : expiringOfferKeys) {
				auto cacheIter = cache.find(key);
				auto& entry = cacheIter.get();
				entry.markExpiredOffers(context.Height);
			}

			if (NotifyMode::Rollback == context.Mode)
				return;

			const auto& config = pConfigHolder->Config(context.Height);
			auto maxRollbackBlocks = config.Network.MaxRollbackBlocks;
			if (context.Height.unwrap() <= maxRollbackBlocks)
				return;

			auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
			auto expiredOfferKeys = cache.touch(pruneHeight);
			for (const auto& key : expiredOfferKeys) {
				auto cacheIter = cache.find(key);
				auto& entry = cacheIter.get();
				OfferExpiryUpdater offerExpiryUpdater(cache, entry, true);

				Amount xpxAmount(0);
				auto& buyOffers = entry.buyOffers();
				for (auto iter = buyOffers.begin(); iter != buyOffers.end();) {
					if (iter->second.ExpiryHeight <= pruneHeight) {
						xpxAmount = xpxAmount + iter->second.ResidualCost;
						iter = buyOffers.erase(iter);
					} else {
						++iter;
					}
				}
				CreditAccount(entry.owner(), config.Immutable.CurrencyMosaicId, xpxAmount, context);

				auto& sellOffers = entry.sellOffers();
				for (auto iter = sellOffers.begin(); iter != sellOffers.end();) {
					if (iter->second.ExpiryHeight <= pruneHeight) {
						CreditAccount(entry.owner(), iter->first, iter->second.Amount, context);
						iter = sellOffers.erase(iter);
					} else {
						++iter;
					}
				}
			}
		}))
	}
}}
