/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/model/Address.h"
#include <boost/lexical_cast.hpp>

namespace catapult { namespace observers {

	namespace {
		Height GetOfferDeadline(const BlockDuration& duration, const Height& currentHeight) {
			auto maxDeadline = std::numeric_limits<Height::ValueType>::max();
			return Height((duration.unwrap() < maxDeadline - currentHeight.unwrap()) ? currentHeight.unwrap() + duration.unwrap() : maxDeadline);
		}

        SdaOfferBalanceResult ModifyOffer(state::SdaOfferBalanceMap& existingOffer, state::SdaOfferBalanceMap& offerToExchange, const state::MosaicsPair& mosaicIdOfExistingOffer, const state::MosaicsPair& mosaicIdOfOfferToExchange) {
            auto& offerExchange = offerToExchange.at(mosaicIdOfOfferToExchange);
            auto& offer = existingOffer.at(mosaicIdOfExistingOffer);
            
			Amount mosaicExchange1 = offerExchange.CurrentMosaicGet <= offer.CurrentMosaicGive ? offerExchange.CurrentMosaicGet : offer.CurrentMosaicGive;
            Amount mosaicExchange2 = offerExchange.CurrentMosaicGive <= offer.CurrentMosaicGet ? offerExchange.CurrentMosaicGive : offer.CurrentMosaicGet;
            
            offerExchange.CurrentMosaicGive = offerExchange.CurrentMosaicGive - mosaicExchange2;
            offerExchange.CurrentMosaicGet = offerExchange.CurrentMosaicGet - mosaicExchange1;

            offer.CurrentMosaicGive = offer.CurrentMosaicGive - mosaicExchange1;
            offer.CurrentMosaicGet = offer.CurrentMosaicGet - mosaicExchange2;

            SdaOfferBalanceResult result;
            result.OfferPair = BalancePair{offer, offerExchange};
            result.MosaicGiveExchanged = mosaicExchange2;
            result.MosaicGetExchanged = mosaicExchange1;

            return result;
		}
	}

    DEFINE_OBSERVER(PlaceSdaExchangeOfferV1, model::PlaceSdaOfferNotification<1>, [](const model::PlaceSdaOfferNotification<1>& notification, ObserverContext& context) {
        auto &cache = context.Cache.sub<cache::SdaExchangeCache>();
        if (!cache.contains(notification.Signer))
            cache.insert(state::SdaExchangeEntry(notification.Signer, 1));

        auto iter = cache.find(notification.Signer);
        auto &entry = iter.get();
        SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, entry);

        /// Save offers in cache
        const auto *pSdaOffer = notification.SdaOffersPtr;
        for (uint8_t i = 0; i < notification.SdaOfferCount; ++i, ++pSdaOffer) {
            auto mosaicIdGive = context.Resolvers.resolve(pSdaOffer->MosaicGive.MosaicId);
            auto mosaicIdGet = context.Resolvers.resolve(pSdaOffer->MosaicGet.MosaicId);

            auto deadline = GetOfferDeadline(pSdaOffer->Duration, context.Height);
            entry.addOffer(mosaicIdGive, mosaicIdGet, pSdaOffer, deadline);

            std::string reduced = reducedFraction(pSdaOffer->MosaicGive.Amount, pSdaOffer->MosaicGet.Amount);
            auto groupHash = calculateGroupHash(mosaicIdGive, mosaicIdGet, reduced);

            auto &groupCache = context.Cache.sub<cache::SdaOfferGroupCache>();
            if (!groupCache.contains(groupHash))
                groupCache.insert(state::SdaOfferGroupEntry(groupHash));

            auto groupIter = groupCache.find(groupHash);
            auto &groupEntry = groupIter.get();

            groupEntry.addSdaOfferToGroup(pSdaOffer, notification.Signer, deadline);

            model::OfferCreationReceipt creationReceipt(model::Receipt_Type_Sda_Offer_Created, entry.owner(), state::MosaicsPair(mosaicIdGive, mosaicIdGet), pSdaOffer->MosaicGive.Amount, pSdaOffer->MosaicGet.Amount);
            context.StatementBuilder().addTransactionReceipt(creationReceipt);
        }

        /// Exchange offers when a match is found in cache
        const auto &pluginConfig = context.Config.Network.template GetPluginConfiguration<config::SdaExchangeConfiguration>();
        state::SdaOfferBalanceMap currentOffers =  entry.sdaOfferBalances();
        for (auto &offersBySigner: currentOffers) {
            // Get groupEntry of the current offer
            std::string reducedOfCurrentOffer = reducedFraction(offersBySigner.second.InitialMosaicGive,
                                                                offersBySigner.second.InitialMosaicGet);
            auto groupHashOfCurrentOffer = calculateGroupHash(offersBySigner.first.first, offersBySigner.first.second,
                                                              reducedOfCurrentOffer);
            auto &groupCacheOfCurrentOffer = context.Cache.sub<cache::SdaOfferGroupCache>();
            auto groupIterOfCurrentOffer = groupCacheOfCurrentOffer.find(groupHashOfCurrentOffer);
            auto &groupEntryOfCurrentOffer = groupIterOfCurrentOffer.get();

            // Current offer's Give is existing offer's Get
            // Current offer's Get is existing offer's Give

            // Get groupEntry of the existing offer
            std::string reducedOfExistingOffer = reducedFraction(offersBySigner.second.InitialMosaicGet,
                                                                 offersBySigner.second.InitialMosaicGive);
            auto groupHashOfExistingOffer = calculateGroupHash(offersBySigner.first.second, offersBySigner.first.first,
                                                               reducedOfExistingOffer);
            auto &groupCacheOfExistingOffer = context.Cache.sub<cache::SdaOfferGroupCache>();
            if (!groupCacheOfExistingOffer.contains(groupHashOfExistingOffer))
                continue;
            auto groupIterOfExistingOffer = groupCacheOfExistingOffer.find(groupHashOfExistingOffer);
            auto &groupEntryOfExistingOffer = groupIterOfExistingOffer.get();

            // Sort existing offers of the same group hash
            state::SdaOfferGroupVector arrangedOffers;
            auto offers = groupEntryOfExistingOffer.sdaOfferGroup();
            switch (pluginConfig.OfferSortPolicy) {
                case SortPolicy::Default:
                    arrangedOffers = offers;
                    break;
                case SortPolicy::SmallToBig:
                    arrangedOffers = groupEntryOfExistingOffer.smallToBig();
                    break;
                case SortPolicy::SmallToBigSortedByEarliestExpiry:
                    arrangedOffers = groupEntryOfExistingOffer.smallToBigSortedByEarliestExpiry();
                    break;
                case SortPolicy::BigToSmall:
                    arrangedOffers = groupEntryOfExistingOffer.bigToSmall();
                    break;
                case SortPolicy::BigToSmallSortedByEarliestExpiry:
                    arrangedOffers = groupEntryOfExistingOffer.bigToSmallSortedByEarliestExpiry();
                    break;
                case SortPolicy::ExactOrClosest:
                    // Current offer's Get is existing offer's Give
                    arrangedOffers = groupEntryOfExistingOffer.exactOrClosest(offersBySigner.second.CurrentMosaicGet);
                    break;
            }

            // Exchange sorted offers not belonging to the signer 
            std::vector<model::ExchangeDetail> details;
            for (auto &offer: arrangedOffers) {
                if (offer.Owner == entry.owner()) continue;

                auto iter = cache.find(offer.Owner);
                auto &existingOffer = iter.get();
                SdaOfferExpiryUpdater sdaOfferExpiryUpdater(cache, existingOffer);
                auto result = ModifyOffer(existingOffer.sdaOfferBalances(), entry.sdaOfferBalances(),
                                          state::MosaicsPair{offersBySigner.first.second, offersBySigner.first.first},
                                          offersBySigner.first);
                groupEntryOfExistingOffer.updateSdaOfferGroup(existingOffer.owner(),
                                                              result.OfferPair.first.CurrentMosaicGet);
                groupEntryOfCurrentOffer.updateSdaOfferGroup(entry.owner(), result.OfferPair.second.CurrentMosaicGive);

                // mosaicIdGive of currentOffer is the mosaicIdGet of existingOffer
                // mosaicIdGet of currentOffer is the mosaicIdGive of existingOffer
                CreditAccount(existingOffer.owner(), offersBySigner.first.first, result.MosaicGiveExchanged, context);
                CreditAccount(entry.owner(), offersBySigner.first.second, result.MosaicGetExchanged, context);

                auto address = model::PublicKeyToAddress(existingOffer.owner(), cache.networkIdentifier());
                details.emplace_back(model::ExchangeDetail{address, state::MosaicsPair(offersBySigner.first.second, offersBySigner.first.first), result.MosaicGetExchanged, result.MosaicGiveExchanged});

                // Check if any existing offer still has balance after the exchange
                if (Amount(0) == result.OfferPair.first.CurrentMosaicGive &&
                    Amount(0) == result.OfferPair.first.CurrentMosaicGet) {
                    existingOffer.expireOffer(state::MosaicsPair{offersBySigner.first.second, offersBySigner.first.first},
                                              context.Height);
                    groupEntryOfExistingOffer.removeSdaOfferFromGroup(existingOffer.owner());
                }

                // Check if any offer belonging to the signer still has balance after the exchange
                if (Amount(0) == result.OfferPair.second.CurrentMosaicGive &&
                    Amount(0) == result.OfferPair.second.CurrentMosaicGet) {
                    entry.expireOffer(offersBySigner.first, context.Height);
                    groupEntryOfCurrentOffer.removeSdaOfferFromGroup(entry.owner());
                    break;
                }
            }

            if (details.size() != 0) {
                model::OfferExchangeReceipt exchangeReceipt(model::Receipt_Type_Sda_Offer_Exchanged, entry.owner(), state::MosaicsPair(offersBySigner.first.first, offersBySigner.first.second), details);
                context.StatementBuilder().addTransactionReceipt(exchangeReceipt);
            }
        }
    });
}}