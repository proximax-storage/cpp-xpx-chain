/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "src/cache/ExchangeCache.h"

namespace catapult { namespace observers {

	namespace {
		void CreditAccount(
				state::ExchangeEntry& entry,
				model::OfferType offerType,
				const MosaicId& currencyMosaicId,
				const MosaicId& unitMosaicId,
				const ObserverContext &context) {
			Amount amount;
			if (model::OfferType::Buy == offerType) {
				auto& buyOffers = (NotifyMode::Commit == context.Mode) ? entry.buyOffers() : entry.expiredBuyOffers().at(context.Height);
				amount = buyOffers.at(unitMosaicId).ResidualCost;
			} else {
				auto& sellOffers = (NotifyMode::Commit == context.Mode) ? entry.sellOffers() : entry.expiredSellOffers().at(context.Height);
				amount = sellOffers.at(unitMosaicId).Amount;
			}
			auto mosaicId = (model::OfferType::Buy == offerType) ? currencyMosaicId : unitMosaicId;
			CreditAccount(entry.owner(), mosaicId, amount, context);
		}
	}

	DECLARE_OBSERVER(RemoveOfferV1, model::RemoveOfferNotification<1>)(const MosaicId& currencyMosaicId) {
		return MAKE_OBSERVER(RemoveOfferV1, model::RemoveOfferNotification<1>, ([currencyMosaicId](const model::RemoveOfferNotification<1>& notification, const ObserverContext& context) {
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

	DECLARE_OBSERVER(RemoveOfferV2, model::RemoveOfferNotification<2>)(const MosaicId& currencyMosaicId) {
		return MAKE_OBSERVER(RemoveOfferV2, model::RemoveOfferNotification<2>, ([currencyMosaicId](const model::RemoveOfferNotification<2>& notification, const ObserverContext& context) {
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
