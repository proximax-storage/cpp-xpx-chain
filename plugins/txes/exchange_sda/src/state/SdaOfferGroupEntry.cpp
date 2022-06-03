/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaOfferGroupEntry.h"
#include <algorithm>

namespace catapult { namespace state {

    /// Returns offers arranged from the smallest to the biggest MosaicGive amount.
    SdaOfferGroupVector SdaOfferGroupEntry::smallToBig() {
        std::sort(m_sdaOfferGroup.begin(), m_sdaOfferGroup.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) -> bool {
            return a.MosaicGive < b.MosaicGive;
        });

        return m_sdaOfferGroup;
    }

    /// Returns offers arranged from the smallest to the biggest MosaicGive amount by the earliest expiry date.
    SdaOfferGroupVector SdaOfferGroupEntry::smallToBigSortedByEarliestExpiry() {
        std::sort(m_sdaOfferGroup.begin(), m_sdaOfferGroup.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return std::tie(a.Deadline, a.MosaicGive) < std::tie(b.Deadline, b.MosaicGive);
        });

        return m_sdaOfferGroup;
    }

    /// Returns offers arranged from the biggest to the smallest MosaicGive amount.
    SdaOfferGroupVector SdaOfferGroupEntry::bigToSmall() {
        std::sort(m_sdaOfferGroup.begin(), m_sdaOfferGroup.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return a.MosaicGive > b.MosaicGive;
        });

        return m_sdaOfferGroup;
    }
    
    /// Returns offers arranged from the biggest to the smallest MosaicGive amount by the earliest expiry date.
    SdaOfferGroupVector SdaOfferGroupEntry::bigToSmallSortedByEarliestExpiry() {
        std::sort(m_sdaOfferGroup.begin(), m_sdaOfferGroup.end(), [](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return a.Deadline<b.Deadline ? true : a.Deadline==b.Deadline ? a.MosaicGive>b.MosaicGive : false;
        });

        return m_sdaOfferGroup;
    }

    /// Returns offers arranged from the exact or closest MosaicGive amount.
    SdaOfferGroupVector SdaOfferGroupEntry::exactOrClosest(const Amount offerTarget) {
        std::sort(m_sdaOfferGroup.begin(), m_sdaOfferGroup.end(), [offerTarget](SdaOfferBasicInfo a, SdaOfferBasicInfo b) {
            return std::abs(static_cast<int64_t>(offerTarget.unwrap())-static_cast<int64_t>(a.MosaicGive.unwrap())) 
                        < std::abs(static_cast<int64_t>(offerTarget.unwrap())-static_cast<int64_t>(b.MosaicGive.unwrap()));
        });

        return m_sdaOfferGroup;
    }

    void SdaOfferGroupEntry::addSdaOfferToGroup(const model::SdaOfferWithOwnerAndDuration* pOffer, const Height& deadline) {
        state::SdaOfferBasicInfo offer{ pOffer->Owner, pOffer->MosaicGive.Amount, deadline };
        m_sdaOfferGroup.emplace_back(offer);
    }

    void SdaOfferGroupEntry::removeSdaOfferFromGroup(const Key& offerOwner) {
        for (auto i = 0; i < m_sdaOfferGroup.size(); ++i) {
            if (m_sdaOfferGroup[i].Owner==offerOwner) m_sdaOfferGroup.erase(m_sdaOfferGroup.begin()+i);
        }
    }

    bool SdaOfferGroupEntry::empty() const {
        return m_sdaOfferGroup.empty();
    }
}}