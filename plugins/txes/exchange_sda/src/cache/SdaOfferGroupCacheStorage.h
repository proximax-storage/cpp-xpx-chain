/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaOfferGroupCacheTypes.h"
#include "src/state/SdaOfferGroupEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading SDA Offer Group cache data.
	struct SdaOfferGroupCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<SdaOfferGroupCacheDescriptor>
			, public state::SdaOfferGroupEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
