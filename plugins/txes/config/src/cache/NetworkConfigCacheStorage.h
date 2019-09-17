/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "NetworkConfigCacheTypes.h"
#include "plugins/txes/config/src/state/NetworkConfigEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading network config cache data.
	struct NetworkConfigCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<NetworkConfigCacheDescriptor>
			, public state::NetworkConfigEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
