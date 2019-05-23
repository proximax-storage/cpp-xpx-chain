/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "CatapultUpgradeCacheTypes.h"
#include "src/state/CatapultUpgradeEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading catapult upgrade cache data.
	struct CatapultUpgradeCacheStorage
			: public CacheStorageFromDescriptor<CatapultUpgradeCacheDescriptor>
			, public state::CatapultUpgradeEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
