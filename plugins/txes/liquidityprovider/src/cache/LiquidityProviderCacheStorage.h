/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "LiquidityProviderCacheTypes.h"
#include "src/state/LiquidityProviderEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading LiquidityProvider cache data.
	struct LiquidityProviderCacheStorage
		: public CacheStorageForBasicInsertRemoveCache<LiquidityProviderCacheDescriptor>
			, public state::LiquidityProviderEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
