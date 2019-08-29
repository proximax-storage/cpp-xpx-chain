/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BlockchainUpgradeCacheTypes.h"
#include "src/state/BlockchainUpgradeEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading catapult upgrade cache data.
	struct BlockchainUpgradeCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<BlockchainUpgradeCacheDescriptor>
			, public state::BlockchainUpgradeEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
