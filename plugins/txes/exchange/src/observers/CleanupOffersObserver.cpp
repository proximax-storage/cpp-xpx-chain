/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include "src/cache/OfferCache.h"

namespace catapult { namespace observers {

	void CreditAccount(const Key& recipient, const model::UnresolvedMosaic& mosaic, const ObserverContext& context) {
		if (Amount(0) == mosaic.Amount)
			return;

		auto& cache = context.Cache.sub<cache::AccountStateCache>();
		auto& recipientState = cache.find(recipient).get();
		auto mosaicId = context.Resolvers.resolve(mosaic.MosaicId);
		recipientState.Balances.credit(mosaicId, mosaic.Amount, context.Height);
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

			auto currencyMosaicId = config::GetUnresolvedCurrencyMosaicId(config.Immutable);
			auto& offerCache = context.Cache.sub<cache::OfferCache>();
			auto pruneHeight = Height(context.Height.unwrap() - maxRollbackBlocks);
			auto expiredOfferHashes = offerCache.touch(pruneHeight);
			for (const auto& hash : expiredOfferHashes) {
				state::OfferEntry& offerEntry = offerCache.find(hash).get();
				if (offerEntry.offerType() == model::OfferType::Buy) {
					model::UnresolvedMosaic mosaic{currencyMosaicId, Amount(0)};
					for (const auto& pair : offerEntry.offers())
						mosaic.Amount = Amount(mosaic.Amount.unwrap() + pair.second.Cost.unwrap());
					CreditAccount(offerEntry.transactionSigner(), mosaic, context);
				} else {
					for (const auto& pair : offerEntry.offers())
						CreditAccount(offerEntry.transactionSigner(), pair.second.Mosaic, context);
				}
			}
			offerCache.prune(pruneHeight);
		}))
	}
}}
