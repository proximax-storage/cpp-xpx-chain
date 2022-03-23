/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#include "Observers.h"
#include "catapult/cache_core/AccountStateCache.h"
#include <boost/lexical_cast.hpp>

namespace catapult { namespace observers {

	SdaOfferExpiryUpdater::~SdaOfferExpiryUpdater() {
		const auto& owner = m_entry.owner();
		auto minExpiryHeight = m_entry.minExpiryHeight();
		auto minPruneHeight = m_entry.minPruneHeight();

		m_cache.addExpiryHeight(owner, minPruneHeight);
		m_cache.addExpiryHeight(owner, minExpiryHeight);
	}

	void CreditAccount(const Key& owner, const MosaicId& mosaicId, const Amount& amount, const ObserverContext &context) {
		if (Amount(0) != amount) {
			auto &cache = context.Cache.sub<cache::AccountStateCache>();
			auto iter = cache.find(owner);
			auto &recipientState = iter.get();
			recipientState.Balances.credit(mosaicId, amount, context.Height);
		}
	}

	int denominator(int mosaicGive, int mosaicGet) {
        return (mosaicGet == 0) ? mosaicGive: denominator(mosaicGet, mosaicGive % mosaicGet);
    };

    std::string reducedFraction(Amount mosaicGive, Amount mosaicGet) {
        int denom = denominator(static_cast<int>(mosaicGive.unwrap()), static_cast<int>(mosaicGet.unwrap()));
        int mosaicGiveReduced = static_cast<int>(mosaicGive.unwrap())/denom;
        int mosaicGetReduced = static_cast<int>(mosaicGet.unwrap())/denom;
        return boost::lexical_cast<std::string>(mosaicGiveReduced) + "/" + boost::lexical_cast<std::string>(mosaicGetReduced);
    }

    Hash256 calculateGroupHash(MosaicId mosaicGiveId, MosaicId mosaicGetId, std::string reduced) {
        std::string key = boost::lexical_cast<std::string>(mosaicGiveId) + boost::lexical_cast<std::string>(mosaicGetId) + reduced;
        Hash256 groupHash;
        std::memcpy(groupHash.data(), key.data(), Hash256_Size);

        return groupHash;
    }
}}
