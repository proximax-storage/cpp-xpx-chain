/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "catapult/model/NotificationSubscriber.h"
#include "catapult/observers/ObserverTypes.h"
#include "src/cache/DealCache.h"
#include "src/model/ExchangeNotifications.h"
#include "src/state/OfferEntry.h"

namespace catapult { namespace observers {

	template<typename TNotification, typename TCache, bool BuyOffer>
	void OfferObserver(const TNotification& notification, const ObserverContext& context) {
		auto& cache = context.Cache.sub<TCache>();
		if (NotifyMode::Commit == context.Mode) {
			state::OfferEntry entry(notification.TransactionHash, notification.Signer);
			entry.setDeadline(notification.Deadline);
			auto pOffer = notification.OffersPtr;
			Amount deposit(0);
			for (uint8_t i = 0; i < notification.OfferCount; ++i, ++pOffer) {
				entry.offers().emplace(pOffer->Mosaic.MosaicId, *pOffer);
				deposit = Amount(deposit.unwrap() + pOffer->Cost.unwrap());
			}
			if (BuyOffer)
				entry.setDeposit(deposit);

			cache.insert(entry);
		} else {
			cache.remove(notification.TransactionHash);
		}
	}

	/// Observes changes triggered by buy offer notifications
	DECLARE_OBSERVER(BuyOffer, model::BuyOfferNotification<1>)();

	/// Observes changes triggered by sell offer notifications
	DECLARE_OBSERVER(SellOffer, model::SellOfferNotification<1>)();

	template<typename TEntry, typename TCache>
	void CleanupOffers(TEntry& entry, TCache& cache, UnresolvedMosaicId currencyMosaicId, model::NotificationSubscriber& sub) {
		entry.cleanupOffers();
		if (entry.offers().empty()) {
			if (entry.deposit() > Amount(0))
				sub.notify(model::BalanceCreditNotification<1>(entry.transactionSigner(), currencyMosaicId, entry.deposit()));
			cache.remove(entry.transactionHash());
		}
	}

	template<typename TNotification, typename TSuggestedOfferCache, typename TAcceptedOfferCache>
	void MatchedOfferObserver(const TNotification& notification, const ObserverContext& context) {
		auto& suggestedOfferCache = context.Cache.sub<TSuggestedOfferCache>();
		auto& acceptedOfferCache = context.Cache.sub<TAcceptedOfferCache>();
		auto& dealCache = context.Cache.sub<cache::DealCache>();

		if (NotifyMode::Commit == context.Mode) {
			auto& suggestedOfferEntry = suggestedOfferCache.find(notification.TransactionHash).get();

			{
				state::DealEntry dealEntry(notification.TransactionHash);
				dealEntry.setDeposit(suggestedOfferEntry.deposit());
				dealCache.insert(dealEntry);
			}
			auto& dealEntry = dealCache.find(notification.TransactionHash).get();

			auto pMatchedOffer = notification.MatchedOffersPtr;
			for (uint8_t i = 0; i < notification.MatchedOfferCount; ++i, ++pMatchedOffer) {
				const auto& mosaicId = pMatchedOffer->Mosaic.MosaicId;
				const auto& transactionHash = pMatchedOffer->TransactionHash;

				if (suggestedOfferEntry.deposit() > Amount(0))
					suggestedOfferEntry.decreaseDeposit(pMatchedOffer->Cost);

				dealEntry.acceptedDeals()[transactionHash].emplace(mosaicId, *pMatchedOffer);

				auto& suggestedOffer = suggestedOfferEntry.offers().at(mosaicId);
				auto cost = suggestedOffer.Cost;
				suggestedOffer -= pMatchedOffer->Mosaic.Amount;
				model::Offer deal{pMatchedOffer->Mosaic, Amount(cost.unwrap() - suggestedOffer.Cost.unwrap())};
				dealEntry.suggestedDeals().emplace(mosaicId, deal);

				auto& acceptedOfferEntry = acceptedOfferCache.find(transactionHash).get();
				acceptedOfferEntry.offers().at(mosaicId) -= *pMatchedOffer;
				if (acceptedOfferEntry.deposit() > Amount(0)) {
					dealEntry.deposits().emplace(transactionHash, acceptedOfferEntry.deposit());
					acceptedOfferEntry.decreaseDeposit(pMatchedOffer->Cost);
				}

				CleanupOffers(acceptedOfferEntry, acceptedOfferCache, notification.CurrencyMosaicId, notification.Subscriber);
			}

			CleanupOffers(suggestedOfferEntry, suggestedOfferCache, notification.CurrencyMosaicId, notification.Subscriber);

		} else {
			auto& dealEntry = dealCache.find(notification.TransactionHash).get();
			bool entryWasRemoved = !suggestedOfferCache.contains(notification.TransactionHash);
			if (entryWasRemoved)
				suggestedOfferCache.insert(state::OfferEntry(notification.TransactionHash, notification.Signer));

			auto& suggestedOfferEntry = suggestedOfferCache.find(notification.TransactionHash).get();
			suggestedOfferEntry.setDeposit(dealEntry.deposit());
			if (entryWasRemoved && dealEntry.deposit() > Amount(0))
				notification.Subscriber.notify(model::BalanceCreditNotification<1>(notification.Signer, notification.CurrencyMosaicId, dealEntry.deposit()));

			auto pMatchedOffer = notification.MatchedOffersPtr;
			for (uint8_t i = 0; i < notification.MatchedOfferCount; ++i, ++pMatchedOffer) {
				const auto& mosaicId = pMatchedOffer->Mosaic.MosaicId;
				const auto& transactionHash = pMatchedOffer->TransactionHash;
				const auto& transactionSigner = pMatchedOffer->TransactionSigner;

				auto& suggestedOffer = suggestedOfferEntry.offers()[mosaicId];
				suggestedOffer += dealEntry.suggestedDeals().at(mosaicId);

				entryWasRemoved = !acceptedOfferCache.contains(transactionHash);
				if (entryWasRemoved)
					acceptedOfferCache.insert(state::OfferEntry(transactionHash, transactionSigner));

				auto& acceptedOfferEntry = acceptedOfferCache.find(transactionHash).get();
				auto deposits = dealEntry.deposits();
				auto depositIter = deposits.find(transactionHash);
				if (depositIter != deposits.end()) {
					acceptedOfferEntry.setDeposit(depositIter->second);
					if (entryWasRemoved)
						notification.Subscriber.notify(model::BalanceCreditNotification<1>(transactionSigner, notification.CurrencyMosaicId, depositIter->second));
				}
				auto& acceptedOffer = acceptedOfferEntry.offers()[mosaicId];
				acceptedOffer += dealEntry.acceptedDeals().at(transactionHash).at(mosaicId);
			}

			dealCache.remove(notification.TransactionHash);
		}
	}

	/// Observes changes triggered by matched buy offer notifications
	DECLARE_OBSERVER(MatchedBuyOffer, model::MatchedBuyOfferNotification<1>)();

	/// Observes changes triggered by matched sell offer notifications
	DECLARE_OBSERVER(MatchedSellOffer, model::MatchedSellOfferNotification<1>)();
}}
