/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaOfferGroupEntry.h"
#include <algorithm>

namespace catapult { namespace state {

    /// Returns offers arranged from the smallest to the biggest MosaicGive amount.
    SdaOfferGroupMap SdaOfferGroupEntry::smallToBig(const Hash256 groupHash, SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return a.MosaicGive < b.MosaicGive;
        });

        return sdaOfferGroup;
    }

    /// Returns offers arranged from the smallest to the biggest MosaicGive amount by the earliest expiry date.
    SdaOfferGroupMap SdaOfferGroupEntry::smallToBigSortedByEarliestExpiry(const Hash256 groupHash, SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return std::tie(a.Deadline, a.MosaicGive) < std::tie(b.Deadline, b.MosaicGive);
        });

        return sdaOfferGroup;
    }

    /// Returns offers arranged from the biggest to the smallest MosaicGive amount.
    SdaOfferGroupMap SdaOfferGroupEntry::bigToSmall(const Hash256 groupHash, SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return a.MosaicGive > b.MosaicGive;
        });

        return sdaOfferGroup;
    }
    
    /// Returns offers arranged from the biggest to the smallest MosaicGive amount by the earliest expiry date.
    SdaOfferGroupMap SdaOfferGroupEntry::bigToSmallSortedByEarliestExpiry(const Hash256 groupHash, SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return a.Deadline<b.Deadline ? true : a.Deadline==b.Deadline ? a.MosaicGive>b.MosaicGive : false;
        });

        return sdaOfferGroup;
    }

    /// Returns offers arranged from the exact or closest MosaicGive amount.
    SdaOfferGroupMap SdaOfferGroupEntry::exactOrClosest(const Hash256 groupHash, const Amount offerTarget, SdaOfferGroupMap& sdaOfferGroup) {
        auto& offer = sdaOfferGroup.find(groupHash)->second;
        std::sort(offer.begin(), offer.end(), [offerTarget](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return std::abs(static_cast<int64_t>(offerTarget.unwrap())-static_cast<int64_t>(a.MosaicGive.unwrap())) 
                        < std::abs(static_cast<int64_t>(offerTarget.unwrap())-static_cast<int64_t>(b.MosaicGive.unwrap()));
        });

        return sdaOfferGroup;
    }

    void SdaOfferGroupEntry::addSdaOfferToGroup(const Hash256& groupHash, const model::SdaOfferWithOwnerAndDuration* pOffer, const Height& deadline) {
        state::SdaOfferBasicInfo offer{ pOffer->Owner, pOffer->MosaicGive.Amount, deadline };
        if (m_sdaOfferGroup.count(groupHash) == 0) {
            std::vector<state::SdaOfferBasicInfo> info;
            info.emplace_back(offer);
            m_sdaOfferGroup.emplace(groupHash, info);
        }
        else {
            (m_sdaOfferGroup.find(groupHash)->second).emplace_back(offer);
        }
    }

    void SdaOfferGroupEntry::removeSdaOfferFromGroup(const Hash256& groupHash, const Key& offerOwner) {
        auto& offer = m_sdaOfferGroup.find(groupHash)->second;
        for (auto i = 0; i < offer.size(); ++i) {
            if (offer[i].Owner==offerOwner) offer.erase(offer.begin()+i);
        }
    }
}}