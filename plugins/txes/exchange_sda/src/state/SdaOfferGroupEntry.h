/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "src/model/SdaOffer.h"
#include "src/catapult/types.h"
#include <map>

namespace catapult { namespace state {

    struct SdaOfferBasicInfo {
        Key Owner;
        catapult::Amount MosaicGive;
        Height Deadline;
    };

    using SdaOfferGroupMap = std::map<Hash256, std::vector<SdaOfferBasicInfo>>;

    // SDA-SDA Offer Group entry.
    class SdaOfferGroupEntry {
    public:
		// Creates an SDA-SDA Offer Groups entry around \a group hash.
		SdaOfferGroupEntry(const Hash256& groupHash)
			: m_groupHash(groupHash)
		{}

    public:
        /// Gets the group hash of the offers.
		const Hash256& groupHash() const {
			return m_groupHash;
		}

        /// Gets the offers from the same group.
        SdaOfferGroupMap& sdaOfferGroup() {
            return m_sdaOfferGroup;
        }

        /// Gets the offers from the same group.
        const SdaOfferGroupMap& sdaOfferGroup() const {
            return m_sdaOfferGroup;
        }
    
    public:
        /// Returns offers arranged from the smallest to the biggest MosaicGive amount.
        SdaOfferGroupMap smallToBig(const Hash256 groupHash, const SdaOfferGroupMap& sdaOfferGroup);

        /// Returns offers arranged from the smallest to the biggest MosaicGive amount by the earliest expiry date.
        SdaOfferGroupMap smallToBigSortedByEarliestExpiry(const Hash256 groupHash, const SdaOfferGroupMap& sdaOfferGroup);

        /// Returns offers arranged from the biggest to the smallest MosaicGive amount.
        SdaOfferGroupMap bigToSmall(const Hash256 groupHash, const SdaOfferGroupMap& sdaOfferGroup);
        
        /// Returns offers arranged from the biggest to the smallest MosaicGive amount by the earliest expiry date.
        SdaOfferGroupMap bigToSmallSortedByEarliestExpiry(const Hash256 groupHash, const SdaOfferGroupMap& sdaOfferGroup);

        /// Returns offers arranged from the exact or closest MosaicGive amount.
        SdaOfferGroupMap exactOrClosest(const Hash256 groupHash, const Amount offer, const SdaOfferGroupMap& sdaOfferGroup);

        void addSdaOfferToGroup(const Hash256& groupHash, const model::SdaOfferWithOwnerAndDuration* pOffer, const Height& deadline);
        void removeSdaOfferFromGroup(const Hash256& groupHash, const Key& offerOwner);

    private:
		Hash256 m_groupHash;
        SdaOfferGroupMap m_sdaOfferGroup;
    };
}}