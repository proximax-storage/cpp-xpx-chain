/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "SdaOfferGroupEntry.h"
#include <algorithm>

namespace catapult { namespace state {

    /// Returns offers arranged from the smallest to the biggest MosaicGive amount.
    SdaOfferGroupMap SdaOfferGroupEntry::smallToBig(const SdaOfferGroupMap& sdaOfferGroup) {
        std::sort(sdaOfferGroup.begin(), sdaOfferGroup.end(), [](SdaOffer a, SdaOffer b) {
            return a.MosaicGive < b.MosaicGive;
        });

        return sdaOfferGroup;
    }

    /// Returns offers arranged from the smallest to the biggest MosaicGive amount by the earliest expiry date.
    SdaOfferGroupMap SdaOfferGroupEntry::smallToBigSortedByEarliestExpiry(const SdaOfferGroupMap& sdaOfferGroup) {
        std::sort(sdaOfferGroup.begin(), sdaOfferGroup.end(), [](SdaOffer a, SdaOffer b) {
            return std::tie(a.Deadline, a.MosaicGive) < std::tie(b.Deadline, b.MosaicGive);
        });

        return sdaOfferGroup;
    }

    /// Returns offers arranged from the biggest to the smallest MosaicGive amount.
    SdaOfferGroupMap SdaOfferGroupEntry::bigToSmall(const SdaOfferGroupMap& sdaOfferGroup) {
        std::sort(sdaOfferGroup.begin(), sdaOfferGroup.end(), [](SdaOffer a, SdaOffer b) {
            return a.MosaicGive > b.MosaicGive;
        });

        return sdaOfferGroup;
    }
    
    /// Returns offers arranged from the biggest to the smallest MosaicGive amount by the earliest expiry date.
    SdaOfferGroupMap SdaOfferGroupEntry::bigToSmallSortedByEarliestExpiry(const SdaOfferGroupMap& sdaOfferGroup) {
        std::sort(sdaOfferGroup.begin(), sdaOfferGroup.end(), [](SdaOffer a, SdaOffer b) {
            return a.Deadline<b.Deadline ? true : a.Deadline==b.Deadline ? a.MosaicGive>b.MosaicGive : false;
        });

        return sdaOfferGroup;
    }

    /// Returns offers arranged from the exact or closest MosaicGive amount.
    SdaOfferGroupMap SdaOfferGroupEntry::exactOrClosest(const SdaOffer offer, const SdaOfferGroupMap& sdaOfferGroup) {
        std::sort(sdaOfferGroup.begin(), sdaOfferGroup.end(), [offer](SdaOffer a, SdaOffer b) {
            return std::abs(static_cast<int64_t>(offer.MosaicGive.unwrap())-static_cast<int64_t>(a.MosaicGive.unwrap())) 
                        < std::abs(static_cast<int64_t>(offer.MosaicGive.unwrap())-static_cast<int64_t>(b.MosaicGive.unwrap()));
        });

        return sdaOfferGroup;
    }

}}