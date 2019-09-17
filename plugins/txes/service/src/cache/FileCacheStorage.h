/**
*** Copyright 2019 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "FileCacheTypes.h"
#include "src/state/FileEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading file cache data.
	struct FileCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<FileCacheDescriptor>
			, public state::FileEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
