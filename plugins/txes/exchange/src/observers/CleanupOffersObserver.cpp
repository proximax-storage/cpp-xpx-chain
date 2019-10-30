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
		auto& recipientState = cache.find(recipient).get();
		recipientState.Balances.credit(mosaicId, amount, context.Height);
	}

	using Notification = model::BlockNotification<1>;

	DECLARE_OBSERVER(CleanupOffers, Notification)(const std::shared_ptr<config::BlockchainConfigurationHolder>& pConfigHolder) {
		return MAKE_OBSERVER(CleanupOffers, Notification, ([pConfigHolder](const Notification&, const ObserverContext& context) {
			if (NotifyMode::Rollback == context.Mode)
				return;

			const auto& config = pConfigHolder->Config(context.Height);
			auto maxRollbackBlocks = config.Network.MaxRollbackBlocks;
			if (context.Height.unwrap() <= maxRollbackBlocks)
				return;

			auto currencyMosaicId = context.Resolvers.resolve(config::GetUnresolvedCurrencyMosaicId(config.Immutable));
			auto& offerCache = context.Cache.sub<cache::ExchangeCache>();
			auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
			auto expiredOfferKeys = offerCache.touch(pruneHeight);
			for (const auto& key : expiredOfferKeys) {
				state::ExchangeEntry& entry = offerCache.find(key).get();

				Amount xpxAmount(0);
				for (const auto& pair : entry.buyOffers())
					xpxAmount = xpxAmount + pair.second.ResidualCost;
				CreditAccount(entry.owner(), currencyMosaicId, xpxAmount, context);

				for (const auto& pair : entry.sellOffers())
					CreditAccount(entry.owner(), pair.first, pair.second.Amount, context);
			}
			offerCache.prune(pruneHeight);
		}))
	}
}}
