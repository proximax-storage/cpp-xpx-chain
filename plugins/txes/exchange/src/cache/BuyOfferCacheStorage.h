/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BuyOfferCacheTypes.h"
#include "src/state/OfferEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading buy offer cache data.
	struct BuyOfferCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<BuyOfferCacheDescriptor>
			, public state::OfferEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
