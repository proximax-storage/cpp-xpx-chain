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
		void CreditAccount(const Key &recipient, const MosaicId &mosaicId, const Amount &amount, const ObserverContext &context) {
			if (Amount(0) == amount)
				return;

			auto &cache = context.Cache.sub<cache::AccountStateCache>();
			auto iter = cache.find(recipient);
			auto &recipientState = iter.get();
			recipientState.Balances.credit(mosaicId, amount, context.Height);
		}

		template<typename TExpiredOfferMap, typename TOfferMap>
		void RemoveExpiredOffers(TExpiredOfferMap& expiredOffers, const Height& height, consumer<const typename TOfferMap::const_iterator&> action) {
			if (expiredOffers.count(height)) {
				auto& expiredOffersAtHeight = expiredOffers.at(height);
				for (auto iter = expiredOffersAtHeight.begin(); iter != expiredOffersAtHeight.end();) {
					action(iter);
					iter = expiredOffersAtHeight.erase(iter);
				}
			}
		}
	}

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(CleanupOffers, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(CleanupOffers, Notification, ([pConfigHolder](const Notification&, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::ExchangeCache>();
			auto expiringOfferOwners = cache.expiringOfferOwners(context.Height);
			for (const auto& key : expiringOfferOwners) {
				auto cacheIter = cache.find(key);
				auto& entry = cacheIter.get();
				OfferExpiryUpdater offerExpiryUpdater(cache, entry);
				if (NotifyMode::Commit == context.Mode) {
					entry.expireOffers(context.Height);
				} else {
					entry.unexpireOffers(context.Height);
				}
			}

			if (NotifyMode::Rollback == context.Mode)
				return;

			const auto& config = pConfigHolder->Config(context.Height);
			auto maxRollbackBlocks = config.Network.MaxRollbackBlocks;
			if (context.Height.unwrap() <= maxRollbackBlocks)
				return;

			auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
			expiringOfferOwners = cache.expiringOfferOwners(pruneHeight);
			for (const auto& key : expiringOfferOwners) {
				auto cacheIter = cache.find(key);
				auto& entry = cacheIter.get();
				OfferExpiryUpdater offerExpiryUpdater(cache, entry);

				Amount xpxAmount(0);
				RemoveExpiredOffers<state::ExpiredBuyOfferMap, state::BuyOfferMap>(entry.expiredBuyOffers(), context.Height, [&xpxAmount](const auto& iter) {
					xpxAmount = xpxAmount + iter->second.ResidualCost;
				});
				CreditAccount(entry.owner(), config.Immutable.CurrencyMosaicId, xpxAmount, context);

				RemoveExpiredOffers<state::ExpiredSellOfferMap, state::SellOfferMap>(entry.expiredSellOffers(), context.Height, [&entry, &context](const auto& iter) {
					CreditAccount(entry.owner(), iter->first, iter->second.Amount, context);
				});
			}
		}))
	}
}}
