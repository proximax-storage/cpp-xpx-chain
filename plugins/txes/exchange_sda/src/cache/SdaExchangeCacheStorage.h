/**
*** Copyright 2022 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "SdaExchangeCacheTypes.h"
#include "src/state/SdaExchangeEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading SDA-SDA exchange cache data.
	struct SdaExchangeCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<SdaExchangeCacheDescriptor>
			, public state::SdaExchangeEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
