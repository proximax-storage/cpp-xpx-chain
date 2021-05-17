/**
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
**/

#pragma once
#include "DownloadChannelCacheTypes.h"
#include "src/state/DownloadChannelEntrySerializer.h"
#include "catapult/cache/CacheStorageInclude.h"

namespace catapult { namespace cache {

	/// Policy for saving and loading download cache data.
	struct DownloadChannelCacheStorage
		: public CacheStorageForBasicInsertRemoveCache<DownloadChannelCacheDescriptor>
			, public state::DownloadChannelEntrySerializer {
		/// Loads \a entry into \a cacheDelta.
		static void LoadInto(const ValueType& entry, DestinationType& cacheDelta);
	};
}}
