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
		void CreditAccount(state::ExchangeEntry& entry, model::OfferType offerType, const MosaicId& currencyMosaicId, const MosaicId& unitMosaicId, const ObserverContext &context) {
			auto amount = (model::OfferType::Buy == offerType) ?
				entry.buyOffers().at(unitMosaicId).ResidualCost :
				entry.sellOffers().at(unitMosaicId).Amount;
			if (Amount(0) != amount) {
				auto &cache = context.Cache.sub<cache::AccountStateCache>();
				auto iter = cache.find(entry.owner());
				auto &recipientState = iter.get();
				auto mosaicId = (model::OfferType::Buy == offerType) ? currencyMosaicId : unitMosaicId;
				if (NotifyMode::Commit == context.Mode)
					recipientState.Balances.credit(mosaicId, amount, context.Height);
				else
					recipientState.Balances.debit(mosaicId, amount, context.Height);
			}
		}
	}
	
	using Notification = model::RemoveOfferNotification<1>;

	DECLARE_OBSERVER(RemoveOffer, Notification)(const MosaicId& currencyMosaicId) {
		return MAKE_OBSERVER(RemoveOffer, Notification, ([currencyMosaicId](const Notification& notification, const ObserverContext& context) {
			auto& cache = context.Cache.sub<cache::ExchangeCache>();
			auto iter = cache.find(notification.Owner);
			auto& entry = iter.get();
			OfferExpiryUpdater offerExpiryUpdater(cache, entry);

			auto pOffer = notification.OffersPtr;
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				auto mosaicId = context.Resolvers.resolve(pOffer->MosaicId);
				CreditAccount(entry, pOffer->OfferType, currencyMosaicId, mosaicId, context);
				if (NotifyMode::Commit == context.Mode)
					entry.expireOffer(pOffer->OfferType, mosaicId, context.Height);
				else
					entry.unexpireOffer(pOffer->OfferType, mosaicId, context.Height);
			}
		}))
	}
}}
