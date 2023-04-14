/**
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DbrbViewCacheTypes.h"
#include "src/state/DbrbProcessEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading DBRB view cache data.
	struct DbrbViewCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<DbrbViewCacheDescriptor>
			, public state::DbrbProcessEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
