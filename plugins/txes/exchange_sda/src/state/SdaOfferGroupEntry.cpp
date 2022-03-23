/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaOfferGroupEntry.h"
#include <algorithm>

namespace catapult { namespace state {

    /// Returns offers arranged from the smallest to the biggest MosaicGive amount.
    SdaOfferGroupMap SdaOfferGroupEntry::smallToBig(const Hash256 groupHash, const SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return a.MosaicGive < b.MosaicGive;
        });

        return sdaOfferGroup;
    }

    /// Returns offers arranged from the smallest to the biggest MosaicGive amount by the earliest expiry date.
    SdaOfferGroupMap SdaOfferGroupEntry::smallToBigSortedByEarliestExpiry(const Hash256 groupHash, const SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return std::tie(a.Deadline, a.MosaicGive) < std::tie(b.Deadline, b.MosaicGive);
        });

        return sdaOfferGroup;
    }

    /// Returns offers arranged from the biggest to the smallest MosaicGive amount.
    SdaOfferGroupMap SdaOfferGroupEntry::bigToSmall(const Hash256 groupHash, const SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return a.MosaicGive > b.MosaicGive;
        });

        return sdaOfferGroup;
    }
    
    /// Returns offers arranged from the biggest to the smallest MosaicGive amount by the earliest expiry date.
    SdaOfferGroupMap SdaOfferGroupEntry::bigToSmallSortedByEarliestExpiry(const Hash256 groupHash, const SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return a.Deadline<b.Deadline ? true : a.Deadline==b.Deadline ? a.MosaicGive>b.MosaicGive : false;
        });

        return sdaOfferGroup;
    }

    /// Returns offers arranged from the exact or closest MosaicGive amount.
    SdaOfferGroupMap SdaOfferGroupEntry::exactOrClosest(const Hash256 groupHash, const SdaOfferBasicInfo offerTarget, const SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [offerTarget](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return std::abs(static_cast<int64_t>(offerTarget.MosaicGive.unwrap())-static_cast<int64_t>(a.MosaicGive.unwrap())) 
                        < std::abs(static_cast<int64_t>(offerTarget.MosaicGive.unwrap())-static_cast<int64_t>(b.MosaicGive.unwrap()));
        });

        return sdaOfferGroup;
    }

    void SdaOfferGroupEntry::addSdaOfferToGroup(const Hash256& groupHash, const model::SdaOfferWithOwnerAndDuration* pOffer, const Height& deadline) {
        state::SdaOfferBasicInfo offer{ pOffer->Owner, pOffer->MosaicGive.Amount, deadline };
        m_sdaOfferGroup.emplace(groupHash, offer);
    }

    void SdaOfferGroupEntry::removeSdaOfferFromGroup(const Hash256& groupHash, const Key& offerOwner) {
        auto& offer = m_sdaOfferGroup.find(groupHash)->second;
        for (auto it = offer.begin(); it != offer.end(); ++it) {
            if (it->Owner==offerOwner) offer.erase(it);
        }
    }
}}