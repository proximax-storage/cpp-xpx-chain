/**
*** Copyright 2024 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BootKeyReplicatorCacheTypes.h"
#include "src/state/BootKeyReplicatorEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	struct BootKeyReplicatorCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<BootKeyReplicatorCacheDescriptor>
			, public state::BootKeyReplicatorEntrySerializer {
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
