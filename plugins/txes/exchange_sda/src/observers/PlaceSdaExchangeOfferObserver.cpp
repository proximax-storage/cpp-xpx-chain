/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include <boost/lexical_cast.hpp>

namespace catapult { namespace observers {

	namespace {
		Height GetOfferDeadline(const BlockDuration& duration, const Height& currentHeight) {
			auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
			return Height((duration.unwrap() < maxDeadline - currentHeight.unwrap()) ? currentHeight.unwrap() + duration.unwrap() : maxDeadline);
		}

        SdaOfferExpiryUpdater::BalancePair ModifyOffer(state::SdaOfferBalanceMap& existingOffer, state::SdaOfferBalanceMap& offerToExchange, const state::MosaicsPair mosaicId) {
            auto& offer = existingOffer.at(mosaicId);
            auto& offerExchange = offerToExchange.at(mosaicId);
            
			Amount mosaicExchange1 = offerExchange.CurrentMosaicGet <= offer.CurrentMosaicGive ? offerExchange.CurrentMosaicGet : offer.CurrentMosaicGive;
            Amount mosaicExchange2 = offerExchange.CurrentMosaicGive <= offer.CurrentMosaicGet ? offerExchange.CurrentMosaicGive : offer.CurrentMosaicGet;
            
            offerExchange += mosaicExchange1;
            offerExchange -= mosaicExchange1;

            offer += mosaicExchange2;
            offer -= mosaicExchange2;
            
            return SdaOfferExpiryUpdater::BalancePair{offer, offerExchange};
		}
	}

    DEFINE_OBSERVER(PlaceSdaExchangeOfferV1, model::PlaceSdaOfferNotification<1>, [](const model::PlaceSdaOfferNotification<1>& notification, const ObserverContext& context) {
        auto& cache = context.Cache.sub<cache::SdaExchangeCache>();
        if (!cache.contains(notification.Signer))
            cache.insert(state::SdaExchangeEntry(notification.Signer, 1));
        
        auto iter = cache.find(notification.Signer);
        auto& entry = iter.get();
        SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, entry);

        /// Save offers in cache
        const auto* pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicIdGive = context.Resolvers.resolve(pSdaOffer->MosaicGive.MosaicId);
            auto mosaicIdGet = context.Resolvers.resolve(pSdaOffer->MosaicGet.MosaicId);

            auto deadline = GetOfferDeadline(pSdaOffer->Duration, context.Height);
            entry.addOffer(mosaicIdGive, mosaicIdGet, pSdaOffer, deadline);
            
            std::string reduced = reducedFraction(pSdaOffer->MosaicGive.Amount, pSdaOffer->MosaicGet.Amount);
            auto groupHash = calculateGroupHash(mosaicIdGive, mosaicIdGet, reduced);

            auto& groupCache = context.Cache.sub<cache::SdaOfferGroupCache>();
            if (!groupCache.contains(groupHash))
                groupCache.insert(state::SdaOfferGroupEntry(groupHash));

            auto groupIter = groupCache.find(groupHash);
            auto& groupEntry = groupIter.get();

            groupEntry.addSdaOfferToGroup(groupHash, pSdaOffer, deadline);
        }

        /// Exchange offers when a match is found in cache
        const auto& pluginConfig = context.Config.Network.template GetPluginConfiguration<config::SdaExchangeConfiguration>();
        for (auto&offersBySigner : entry.sdaOfferBalances()) {
            auto mosaicIdGive = offersBySigner.first.first;
            auto mosaicIdGet = offersBySigner.first.second;

            // Sort existing offers of the same group hash
            std::string reduced = reducedFraction(offersBySigner.second.CurrentMosaicGive, offersBySigner.second.CurrentMosaicGet);
            auto groupHash = calculateGroupHash(mosaicIdGive, mosaicIdGet, reduced);

            auto& groupCache = context.Cache.sub<cache::SdaOfferGroupCache>();
            auto groupIter = groupCache.find(groupHash);
            auto& groupEntry = groupIter.get();

            auto& arrangedOffers = groupEntry.sdaOfferGroup();
            switch (pluginConfig.OfferSortPolicy) {
                case SortPolicy::SmallToBig: 
                    arrangedOffers = groupEntry.smallToBig(groupHash, arrangedOffers);
                    break;
                case SortPolicy::SmallToBigSortedByEarliestExpiry: 
                    arrangedOffers = groupEntry.smallToBigSortedByEarliestExpiry(groupHash, arrangedOffers);
                    break;
                case SortPolicy::BigToSmall: 
                    arrangedOffers = groupEntry.bigToSmall(groupHash, arrangedOffers);
                    break;
                case SortPolicy::BigToSmallSortedByEarliestExpiry: 
                    arrangedOffers = groupEntry.bigToSmallSortedByEarliestExpiry(groupHash, arrangedOffers);
                    break;
                case SortPolicy::ExactOrClosest:
                    arrangedOffers = groupEntry.exactOrClosest(groupHash, offersBySigner.second.CurrentMosaicGive, arrangedOffers);
                    break;
            }

            for (auto& offerList : arrangedOffers) {
                for (auto& offer : offerList.second) {
                    if (offer.Owner == notification.Signer) continue;

                    auto iter = cache.find(offer.Owner);
                    auto& existingOffer = iter.get();
                    auto result = ModifyOffer(existingOffer.sdaOfferBalances(), entry.sdaOfferBalances(), offersBySigner.first);

                    if (Amount(0) == result.first.CurrentMosaicGive && Amount(0) == result.first.CurrentMosaicGet) {
                        existingOffer.expireOffer(offersBySigner.first, context.Height);
                        groupEntry.removeSdaOfferFromGroup(groupHash, existingOffer.owner());
                    }
                    if (Amount(0) == result.second.CurrentMosaicGive && Amount(0) == result.second.CurrentMosaicGet) {
                        entry.expireOffer(offersBySigner.first, context.Height);
                        groupEntry.removeSdaOfferFromGroup(groupHash, entry.owner());
                    }
                }
            }
        }
    });
}}