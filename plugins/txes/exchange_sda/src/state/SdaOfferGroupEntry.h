/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/SdaOffer.h"
#include "src/catapult/types.h"

namespace catapult { namespace state {

    struct SdaOfferBasicInfo {
        Key Owner;
        catapult::Amount MosaicGive;
        Height Deadline;
    };

    using SdaOfferGroupVector = std::vector<SdaOfferBasicInfo>;

    // SDA-SDA Offer Group entry.
    class SdaOfferGroupEntry {
    public:
        // Creates an SDA-SDA Offer Groups entry around \a group hash.
        SdaOfferGroupEntry(const Hash256& groupHash)
            : m_groupHash(groupHash)
        {}

    public:
        /// Sets \a groupHash of SdaOfferBasicInfo.
        void setGroupHash(const Hash256& groupHash) {
            m_groupHash = groupHash;
        }

        /// Gets the group hash of the offers.
        const Hash256& groupHash() const {
            return m_groupHash;
        }

        /// Gets the offers from the same group.
        SdaOfferGroupVector& sdaOfferGroup() {
            return m_sdaOfferGroup;
        }

        /// Gets the offers from the same group.
        const SdaOfferGroupVector& sdaOfferGroup() const {
            return m_sdaOfferGroup;
        }
    
    public:
        /// Returns offers arranged from the smallest to the biggest MosaicGive amount.
        SdaOfferGroupVector smallToBig();

        /// Returns offers arranged from the smallest to the biggest MosaicGive amount by the earliest expiry date.
        SdaOfferGroupVector smallToBigSortedByEarliestExpiry();

        /// Returns offers arranged from the biggest to the smallest MosaicGive amount.
        SdaOfferGroupVector bigToSmall();
        
        /// Returns offers arranged from the biggest to the smallest MosaicGive amount by the earliest expiry date.
        SdaOfferGroupVector bigToSmallSortedByEarliestExpiry();

        /// Returns offers arranged from the exact or closest MosaicGive amount.
        SdaOfferGroupVector exactOrClosest(const Amount offerTarget);

        void addSdaOfferToGroup(const model::SdaOfferWithOwnerAndDuration* pOffer, const Height& deadline);
        void removeSdaOfferFromGroup(const Key& offerOwner);
        bool empty() const;

    private:
        Hash256 m_groupHash;
        SdaOfferGroupVector m_sdaOfferGroup;
    };
}}