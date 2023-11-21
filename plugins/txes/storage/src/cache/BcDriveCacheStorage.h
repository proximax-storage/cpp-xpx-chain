/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "BcDriveCacheTypes.h"
#include "src/state/BcDriveEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading drive cache data.
	struct BcDriveCacheStorage
			: public CacheStorageForBasicInsertRemoveCache<BcDriveCacheDescriptor>
			, public state::BcDriveEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
