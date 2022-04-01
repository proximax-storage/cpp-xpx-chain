/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaExchangeEntry.h"
#include "catapult/exceptions.h"
#include "catapult/utils/Casting.h"
#include <boost/lexical_cast.hpp>
#include <cmath>

namespace catapult { namespace state {

    SdaOfferBalance& SdaOfferBalance::operator+=(const Amount& offerAmount) {
        CurrentMosaicGet = CurrentMosaicGet + offerAmount;
        return *this;
    }

    SdaOfferBalance& SdaOfferBalance::operator-=(const Amount& offerAmount) {
        CurrentMosaicGive = CurrentMosaicGive - offerAmount;
        return *this;
    }

    /// Gets the height of the earliest expiring offer.
    Height SdaExchangeEntry::minExpiryHeight() const {
        if (m_sdaOfferBalances.empty())
            return Invalid_Expiry_Height;

        Height expiryHeight = Height(std::numeric_limits<Height::ValueType>::max());
        std::for_each(m_sdaOfferBalances.begin(), m_sdaOfferBalances.end(), [&expiryHeight](const auto &pair) {
            if (expiryHeight > pair.second.Deadline)
                expiryHeight = pair.second.Deadline;
        });

        return expiryHeight;
    }

    /// Gets the earliest height at which to prune expiring offers.
    Height SdaExchangeEntry::minPruneHeight() const {
        return m_expiredSdaOfferBalances.empty() ? Invalid_Expiry_Height : m_expiredSdaOfferBalances.begin()->first;
    }

    /// Moves pair offer of \a mosaicIdGive and \a mosaicIdGet to expired offer buffer.
    void SdaExchangeEntry::expireOffer(const MosaicsPair& mosaicId, const Height& height) {
        auto expireFunc = [height, mosaicId](SdaOfferBalanceMap& offers, ExpiredSdaOfferBalanceMap& expiredOffers) {
            auto& offer = offers.at(mosaicId);
            auto& expiredOffersAtHeight = expiredOffers[height];
            if (expiredOffersAtHeight.count(mosaicId)) {
                auto pair = boost::lexical_cast<std::string>(mosaicId.first) + ", " + boost::lexical_cast<std::string>(mosaicId.second);
                CATAPULT_THROW_RUNTIME_ERROR_2("offer already expired at height", pair, height);
            }
            expiredOffersAtHeight.emplace(mosaicId, offer);
            offers.erase(mosaicId);
        };
        
        expireFunc(m_sdaOfferBalances, m_expiredSdaOfferBalances);
    }

    /// Moves pair offer of \a mosaicIdGive and \a mosaicIdGet back from expired offer buffer.
    void SdaExchangeEntry::unexpireOffer(const MosaicsPair& mosaicId, const Height& height) {
        auto unexpireFunc = [height, mosaicId](auto& offers, auto& expiredOffers) {
            if (offers.count(mosaicId))
                CATAPULT_THROW_RUNTIME_ERROR_2("offer exists", mosaicId.first, mosaicId.second);
            auto& expiredOffersAtHeight = expiredOffers.at(height);
            auto& offer = expiredOffersAtHeight.at(mosaicId);
            offers.emplace(mosaicId, offer);
            expiredOffersAtHeight.erase(mosaicId);
            if (expiredOffersAtHeight.empty())
                expiredOffers.erase(height);
        };
        
        unexpireFunc(m_sdaOfferBalances, m_expiredSdaOfferBalances);
    }

    /// Moves offers to expired offer buffer if offer expiry height is equal \a height.
    void SdaExchangeEntry::expireOffers(const Height& height, consumer<const SdaOfferBalanceMap::const_iterator&> sdaOfferBalanceAction) {
        for (auto iter = m_sdaOfferBalances.begin(); iter != m_sdaOfferBalances.end();) {
            if (iter->second.Deadline == height) {
                auto &expiredOffersAtHeight = m_expiredSdaOfferBalances[height];
                if (expiredOffersAtHeight.count(iter->first)) {
                    auto pair = boost::lexical_cast<std::string>(iter->first.first) + ", " + boost::lexical_cast<std::string>(iter->first.second);
                    CATAPULT_THROW_RUNTIME_ERROR_2("offer already expired at height", pair, height);
                }
                expiredOffersAtHeight.emplace(iter->first, iter->second);
                sdaOfferBalanceAction(iter);
                iter = m_sdaOfferBalances.erase(iter);
            } else {
                ++iter;
            }
        }
    }

    /// Moves offers back from expired offer buffer if offer expiry height is equal \a height.
    void SdaExchangeEntry::unexpireOffers(const Height& height, consumer<const SdaOfferBalanceMap::const_iterator&> sdaOfferBalanceAction) {
        if (m_expiredSdaOfferBalances.count(height)) {
            auto& expiredOffersAtHeight = m_expiredSdaOfferBalances.at(height);
            for (auto iter = expiredOffersAtHeight.begin(); iter != expiredOffersAtHeight.end();) {
                if (iter->second.Deadline == height) {
                    if (m_sdaOfferBalances.count(iter->first)) {
                        auto pair = boost::lexical_cast<std::string>(iter->first.first) + ", " + boost::lexical_cast<std::string>(iter->first.second);
                        CATAPULT_THROW_RUNTIME_ERROR_2("offer exists at height", pair, height);
                    }
                    m_sdaOfferBalances.emplace(iter->first, iter->second);
                    sdaOfferBalanceAction(m_sdaOfferBalances.find(iter->first));
                    iter = expiredOffersAtHeight.erase(iter);
                } else {
                    ++iter;
                }
            }
            if (expiredOffersAtHeight.empty())
                m_expiredSdaOfferBalances.erase(height);
        }
    }

    bool SdaExchangeEntry::offerExists(const MosaicsPair& mosaicId) const {
        return m_sdaOfferBalances.count(mosaicId);
    }

    void SdaExchangeEntry::addOffer(const MosaicId& mosaicIdGive, const MosaicId& mosaicIdGet, const model::SdaOfferWithOwnerAndDuration* pOffer, const Height& deadline) {
        state::SdaOfferBalance baseOffer{pOffer->MosaicGive.Amount, pOffer->MosaicGet.Amount, pOffer->MosaicGive.Amount, pOffer->MosaicGet.Amount, deadline};
        std::pair<MosaicId, MosaicId> mosaics;
        mosaics.first = mosaicIdGive;
        mosaics.second = mosaicIdGet;
        m_sdaOfferBalances.emplace(mosaics, baseOffer);
    }

    void SdaExchangeEntry::removeOffer(const MosaicsPair& mosaicIds) {
        m_sdaOfferBalances.erase(mosaicIds);
    }

    state::SdaOfferBalance& SdaExchangeEntry::getSdaOfferBalance(const MosaicsPair& mosaicIds) {
        if (!m_sdaOfferBalances.count(mosaicIds))
            CATAPULT_THROW_INVALID_ARGUMENT_2("offer doesn't exist", mosaicIds.first, mosaicIds.second);

        return dynamic_cast<state::SdaOfferBalance&>(m_sdaOfferBalances.at(mosaicIds));
    }

    bool SdaExchangeEntry::empty() const {
        return m_sdaOfferBalances.empty() && m_expiredSdaOfferBalances.empty();
    }
}}
